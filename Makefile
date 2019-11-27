# A generic "build me a kernel module" Makefile comes with FreeBSD, so we just
# need to include it.

KMOD=	rtsz
SRCS=	rtsz.c rtsz_pci.c rtsz.h rtsxreg.h rtsxvar.h device_if.h bus_if.h pci_if.h

.include <bsd.kmod.mk>
