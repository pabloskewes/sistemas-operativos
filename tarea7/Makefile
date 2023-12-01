ccflags-y := -Wall -std=gnu99 -Wno-declaration-after-statement

obj-m := cdown.o

KDIR  := /usr/lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

#include $(KDIR)/.config

install:
	$(MAKE) default
	mknod /dev/cdown c 61 0
	chmod a+rw /dev/cdown
	insmod cdown.ko
	dmesg | tail -1

uninstall:
	rmmod cdown
	rm -rf /dev/cdown

reinstall:
	@echo "Uninstalling module..."
	$(MAKE) uninstall
	@echo "Installing module..."
	$(MAKE) install
	clear
	dmesg | tail -1

print:
	dmesg | tail -50

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
