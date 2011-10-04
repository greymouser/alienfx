obj-m := alienfx.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	-sudo /sbin/rmmod alienfx
	sudo /sbin/insmod alienfx.ko
