# A generic "build me a kernel module" Makefile comes with FreeBSD, so we just
# need to include it.

KMOD=	hellokernel
SRCS=	hellokernel.c

.include <bsd.kmod.mk>
