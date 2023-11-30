ccflags-y := -Wall -std=gnu99 -Wno-declaration-after-statement

obj-m := cdown.o

KDIR  := /usr/lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

#include $(KDIR)/.config

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
