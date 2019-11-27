#include <sys/errno.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/bus.h>

#include "rtsz.h"
#include "rtsxreg.h" /* straight from OpenBSD */

MODULE_VERSION(rtsz, RTSZ_VERSION);

/* internal function prototypes */

static int rtsx_read(struct rtsz_softc *sc, uint16_t addr, uint8_t *val);
static int rtsx_write(struct rtsz_softc *sc, uint16_t addr, uint8_t mask, uint8_t val);

/* sysctls */

SYSCTL_NODE(_hw, OID_AUTO, rtsz, CTLFLAG_RD, 0, "rtsz driver");

static int rtsz_debug = 0;
SYSCTL_INT(
    _hw_rtsz, OID_AUTO, debug, CTLFLAG_RW, &rtsz_debug, 0, "Debug level");

/* implementation */
/* Most of this is taken from the OpenBSD driver. */

/*
 * We use three DMA buffers: a command buffer, a data buffer, and a buffer for
 * ADMA transfer descriptors which describe scatter-gather (SG) I/O operations.
 *
 * The command buffer contains a command queue for the host controller,
 * which describes SD/MMC commands to run, and other parameters. The chip
 * runs the command queue when a special bit in the RTSX_HCBAR register is
 * set and signals completion with the TRANS_OK interrupt.
 * Each command is encoded as a 4 byte sequence containing command number
 * (read, write, or check a host controller register), a register address,
 * and a data bit-mask and value.
 * SD/MMC commands which do not transfer any data from/to the card only use
 * the command buffer.
 *
 * The smmmc stack provides DMA-safe buffers with data transfer commands.
 * In this case we write a list of descriptors to the ADMA descriptor buffer,
 * instructing the chip to transfer data directly from/to sdmmc DMA buffers.
 *
 * However, some sdmmc commands used during card initialization also carry
 * data, and these don't come with DMA-safe buffers. In this case, we transfer
 * data from/to the SD card via a DMA data bounce buffer.
 *
 * In both cases, data transfer is controlled via the RTSX_HDBAR register
 * and completion is signalled by the TRANS_OK interrupt.
 *
 * The chip is unable to perform DMA above 4GB.
 */

#define	RTSX_DMA_MAX_SEGSIZE	0x80000
#define	RTSX_HOSTCMD_MAX	256
#define	RTSX_HOSTCMD_BUFSIZE	(sizeof(u_int32_t) * RTSX_HOSTCMD_MAX)
#define	RTSX_DMA_DATA_BUFSIZE	MAXPHYS
#define	RTSX_ADMA_DESC_SIZE	(sizeof(uint64_t) * SDMMC_MAXNSEGS)

#define READ4(sc, reg)							\
	(bus_space_read_4((sc)->iot, (sc)->ioh, (reg)))
#define WRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->iot, (sc)->ioh, (reg), (val))

int
rtsx_read(struct rtsz_softc *sc, uint16_t addr, uint8_t *val)
{
	int tries = 1024;
	uint32_t reg;
	
	WRITE4(sc, RTSX_HAIMR, RTSX_HAIMR_BUSY |
				 (uint32_t)((addr & 0x3FFF) << 16));

	while (tries--) {
		reg = READ4(sc, RTSX_HAIMR);
		if (!(reg & RTSX_HAIMR_BUSY))
			break;
	}

	*val = (reg & 0xff);
	return (tries == 0) ? ETIMEDOUT : 0;
}

int
rtsx_write(struct rtsz_softc *sc, uint16_t addr, uint8_t mask, uint8_t val)
{
	int tries = 1024;
	uint32_t reg;

	WRITE4(sc, RTSX_HAIMR,
				 RTSX_HAIMR_BUSY | RTSX_HAIMR_WRITE |
				 (uint32_t)(((addr & 0x3FFF) << 16) |
										 (mask << 8) | val));

	while (tries--) {
		reg = READ4(sc, RTSX_HAIMR);
		if (!(reg & RTSX_HAIMR_BUSY)) {
			if (val != (reg & 0xff))
				return EIO;
			return 0;
		}
	}

	return ETIMEDOUT;
}

#define	RTSX_READ(sc, reg, val) 				\
	do { 							\
		int err = rtsx_read((sc), (reg), (val)); 	\
		if (err) 					\
			return (err);				\
	} while (0)

#define	RTSX_WRITE(sc, reg, val) 				\
	do { 							\
		int err = rtsx_write((sc), (reg), 0xff, (val));	\
		if (err) 					\
			return (err);				\
	} while (0)

#define	RTSX_CLR(sc, reg, bits)					\
	do { 							\
		int err = rtsx_write((sc), (reg), (bits), 0); 	\
		if (err) 					\
			return (err);				\
	} while (0)

#define	RTSX_SET(sc, reg, bits)					\
	do { 							\
		int err = rtsx_write((sc), (reg), (bits), 0xff);\
		if (err) 					\
			return (err);				\
	} while (0)

int
rtsz_init(struct rtsz_softc *sc)
{
	uint32_t status;
	uint8_t version;

	/* Read IC version from dummy register. */
	if (sc->flags & RTSX_F_5229) {
		RTSX_READ(sc, RTSX_DUMMY_REG, &version);
		switch (version & 0x0F) {
		case RTSX_IC_VERSION_A:
		case RTSX_IC_VERSION_B:
		case RTSX_IC_VERSION_D:
			break;
		case RTSX_IC_VERSION_C:
			sc->flags |= RTSX_F_5229_TYPE_C;
			break;
		default:
			device_printf(sc->dev, "rtsz_init: unknown IC %02x\n", version);
			return (1);
		}
	}

	/* Enable interrupt write-clear (default is read-clear). */
	RTSX_CLR(sc, RTSX_NFTS_TX_CTRL, RTSX_INT_READ_CLR);
	device_printf(sc->dev, "enable interrupt write-clear: jees\n");

	/* Clear any pending interrupts. */
	status = READ4(sc, RTSX_BIPR);
	WRITE4(sc, RTSX_BIPR, status);
	device_printf(sc->dev, "clear any pending interrupts: jees\n");

	/* Check for cards already inserted at attach time. */
	if (status & RTSX_SD_EXIST) {
	  sc->flags |= RTSX_F_CARD_PRESENT;
	}

	/* TODO the rest :D */

	return (0);
}
