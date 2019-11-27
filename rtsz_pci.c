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
	DEVMETHOD(device_probe, 	rtsz_pci_probe),
	DEVMETHOD(device_attach,	rtsz_pci_attach),
	DEVMETHOD(device_detach,	rtsz_pci_detach),

	DEVMETHOD_END
};

/*
Driver declaration
*/

static driver_t rtsz_driver = {
	"rtsz",
	rtsz_methods,
	sizeof(struct rtsz_softc),
};

static devclass_t rtsz_devclass;
DRIVER_MODULE(rtsz, pci, rtsz_driver, rtsz_devclass, NULL, NULL);

static const struct rtsz_device {
	uint16_t vendor;
	uint16_t device;
	const char *desc;
} rtsz_devices[] = {
	{ 0x10ec, 0x522a, "RTS522A PCI Express SD Card Reader" },
	{ 0x10ec, 0x525a, "RTS525A PCI Express SD Card Reader" },
	{ 0x0000, 0xffff, NULL } /* sentinel */
};

/*
Implementation
*/

#define ISSET(t, f) ((t) & (f))

#define DEBUG(x)  printf("DEBUG (%s at L%d): %s: %d\n", __FUNCTION__, __LINE__, #x, x)

#define	RTSX_F_CARD_PRESENT	0x01

static int
rtsz_pci_probe(device_t dev)
{
	uint16_t vendor;
	uint16_t device;
	int result;

	device = pci_get_device(dev);
	vendor = pci_get_vendor(dev);

	result = ENXIO;
	for (int i = 0; rtsz_devices[i].vendor != 0; i++) {
		if (rtsz_devices[i].device == device &&
		    rtsz_devices[i].vendor == vendor) {
			device_set_desc(dev, rtsz_devices[i].desc);
			return BUS_PROBE_DEFAULT;
		}
	}

	return (result);
}

static int
rtsz_pci_attach(device_t dev)
{
	int err = 0;
	struct rtsz_softc *sc = device_get_softc(dev);
	int bar0id = PCIR_BAR(0);
	/* initialize our softc */
	sc->dev = dev;

	/* allocate memory for the PCI */
	sc->pci_mem_res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &bar0id, RF_ACTIVE);
	if (sc->pci_mem_res == NULL) {
		device_printf(dev, "Memory allocation of PCI base register 0 failed!\n");
		return ENXIO;
	}
	sc->iot = rman_get_bustag(sc->pci_mem_res);
	sc->ioh = rman_get_bushandle(sc->pci_mem_res);
	device_printf(dev, "Got iot 0x%lx, ioh 0x%lx\n", sc->iot, sc->ioh);

	err = rtsz_init(sc);
	if (err) return err;
	device_printf(dev, "CARD_PRESENT: %d\n", ISSET(sc->flags, RTSX_F_CARD_PRESENT));

	device_printf(dev, "what now? :D\n");
	return (EOPNOTSUPP);
}

static int
rtsz_pci_detach(device_t dev)
{
	struct rtsz_softc *sc = device_get_softc(dev);

	bus_release_resource(dev, SYS_RES_MEMORY,
		rman_get_rid(sc->pci_mem_res), sc->pci_mem_res);
  device_printf(dev, "detached\n");
  return (0);
}
