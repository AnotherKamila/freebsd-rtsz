#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

static int event_handler(struct module *module, int event, void *arg);

static int event_handler(struct module *module, int event, void *arg) {
  switch (event) {
  case MOD_LOAD:
    printf("Hello Kernel!\n");
    return 0;
  case MOD_QUIESCE:
  case MOD_UNLOAD:
    printf("Bye Bye\n");
    return 0;
  default:
    return EOPNOTSUPP; // Error, Operation Not Supported
  }
}

static moduledata_t hello_conf = {
    .name = "hellokernel",    /* module name */
    .evhand = event_handler,  /* event handler */
    .priv = NULL            /* extra data */
};

DECLARE_MODULE(hello, hello_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
