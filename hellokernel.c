#include <sys/errno.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysctl.h>

/* style(9) says: "Function prototypes for private functions go at the top of
the first source module." */
static int load_handler(struct module *module, int event, void *arg);

/* see sysctl(9) */
static int sysctltest = 42;
SYSCTL_INT(_debug, OID_AUTO, hello, CTLFLAG_RW, &sysctltest, 0,
    "Testing a static sysctl");

/* see module(9) */
static int
load_handler(struct module *module __attribute__((unused)), int event,
    void *arg __attribute__((unused)))
{
	switch (event) {
	case MOD_LOAD:
		printf("Hello Kernel!\n");
		printf("(also, sysctltest is %d)\n", sysctltest);
		return 0;
	case MOD_QUIESCE:
		printf("Noooooo!!! I don't wanna leaaave :'(\n");
		printf("(also, sysctltest is %d)\n", sysctltest);
		return EBUSY;
	case MOD_UNLOAD:
		printf("Okay okay, I'm gone\n");
		printf("(also, sysctltest is %d)\n", sysctltest);
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

static moduledata_t mod_data = {
	.name   = "hellokernel",
	.evhand = load_handler,
	.priv   = NULL /* TODO what's this? */
};

DECLARE_MODULE(hellokernel, mod_data, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
