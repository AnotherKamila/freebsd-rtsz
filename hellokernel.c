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
    printf("Noooooo!!! I don't wanna leaaave :'(\n");
    return EBUSY;
  case MOD_UNLOAD:
    printf("Okay okay, I'm gone\n");
    return 0;
  default:
    return EOPNOTSUPP;
  }
}

static moduledata_t mod_data = {
    .name   = "hellokernel",
    .evhand = event_handler,
    .priv   = NULL /* TODO what's this? */
};

DECLARE_MODULE(hellokernel, mod_data, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
