# RTSZ driver for new Realtek SD card readers

Yet another attempt to port the OpenBSD RTSX driver to FreeBSD. But I intend to finish this one.

**This is a work in progress!!!** I am literally copy-pasting bits of the OpenBSD code and poking at them until they work. This might fry your laptop or eat your dog.

In order to make this committable, I am following the outline in [this email thread](https://lists.freebsd.org/pipermail/freebsd-hackers/2018-April/052520.html).

## TODO list

TODO convert this to issues :D

- [X] detect card presence on load
- [ ] do the right thing on card insertion
- [ ] card read and write
- [ ] wire it up to the MMC methods
- [ ] add all the devices supported by the OpenBSD driver
- [ ] sort out license + copyright notices in the files
- [ ] get it committed
- [ ] figure out why it's sometimes called RTSX and sometimes RTSZ
