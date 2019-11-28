/*
This is the interface between the PCI parent and the device, the actual
device thingy things go into the main file.
*/
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <machine/resource.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include "rtsz.h"
#include "rtsxreg.h"

/*
 * Device methods.
 */
static int rtsz_pci_probe(device_t dev);
static int rtsz_pci_attach(device_t dev);
static int rtsz_pci_detach(device_t dev);

static device_method_t rtsz_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, rtsz_pci_probe),
	DEVMETHOD(device_attach, rtsz_pci_attach),
	DEVMETHOD(device_detach, rtsz_pci_detach),

	DEVMETHOD_END
};

/*
 * Internal functions.
 */
static void free_bus_mem(struct rtsz_softc *sc);

/*
 * Driver declaration.
 */

static driver_t rtsz_driver = {
	"rtsz",
	rtsz_methods,
	sizeof(struct rtsz_softc),
};

static devclass_t rtsz_devclass;
DRIVER_MODULE(rtsz, pci, rtsz_driver, rtsz_devclass, NULL, NULL);

static const struct rtsz_device {
	uint32_t devid;
	const char *desc;
	int flags;
} rtsz_devices[] = {
	/* TODO support all the devices supported by the OpenBSD driver */
	/* these are the devices I can test, so I'm not adding the rest until I
	   find someone who can test them */
	{ 0x522a10ec, "RTS522A PCI Express SD Card Reader", 0 },
	{ 0x525a10ec, "RTS525A PCI Express SD Card Reader", RTSZ_F_BAR1 },
	{ 0x0, NULL, 0 } /* sentinel */
};

/*
Implementation
*/

#define ISSET(t, f) ((t) & (f))

#define DEBUG(x) \
	printf("DEBUG (%s at L%d): %s: 0x%x\n", __FUNCTION__, __LINE__, #x, x)

static int
rtsz_pci_probe(device_t dev)
{
	uint32_t devid = pci_get_devid(dev);
	int result = ENXIO;

	for (int i = 0; rtsz_devices[i].devid != 0; i++) {
		if (rtsz_devices[i].devid == devid) {
			device_set_desc(dev, rtsz_devices[i].desc);
			return BUS_PROBE_DEFAULT;
		}
	}

	return (result);
}

static int
rtsz_pci_attach(device_t dev)
{
	struct rtsz_softc *sc = device_get_softc(dev);
	uint32_t devid = pci_get_devid(dev);
	int err = 0;
	int bar, rid;

	/* initialize our softc */
	sc->dev = dev;
	for (int i = 0; rtsz_devices[i].devid != 0; i++) {
		if (rtsz_devices[i].devid == devid) {
			sc->flags = rtsz_devices[i].flags;
		}
	}

	/* allocate memory for the PCI */
	bar = ISSET(sc->flags, RTSZ_F_BAR1) ? 1 : 0;
	rid = PCIR_BAR(bar);
	sc->pci_mem_res =
	    bus_alloc_resource_any(dev, SYS_RES_MEMORY, &rid, RF_ACTIVE);
	if (sc->pci_mem_res == NULL) {
		device_printf(
		    dev, "Memory allocation on PCI BAR %d failed!\n", bar);
		return ENXIO;
	}
	sc->iot = rman_get_bustag(sc->pci_mem_res);
	sc->ioh = rman_get_bushandle(sc->pci_mem_res);
	device_printf(dev, "Got iot 0x%lx, ioh 0x%lx\n", sc->iot, sc->ioh);

	err = rtsz_init(sc);
	if (err) {
		free_bus_mem(sc);
		return err;
	}
	device_printf(
	    dev, "CARD_PRESENT: %d\n", ISSET(sc->flags, RTSX_F_CARD_PRESENT));

	device_printf(dev, "what now? :D\n");
	free_bus_mem(sc);
	return (EOPNOTSUPP);
}

static int
rtsz_pci_detach(device_t dev)
{
	struct rtsz_softc *sc = device_get_softc(dev);
	free_bus_mem(sc);
	device_printf(dev, "detached\n");
	return (0);
}

static void
free_bus_mem(struct rtsz_softc *sc)
{
	bus_release_resource(sc->dev, SYS_RES_MEMORY,
	    rman_get_rid(sc->pci_mem_res), sc->pci_mem_res);
}
