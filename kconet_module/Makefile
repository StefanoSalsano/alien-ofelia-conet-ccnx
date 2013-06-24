
ifneq ($(KERNELRELEASE),)

obj-m:= kconet_mod.o
kconet_mod-objs := kconet_server.o kconet.o	
#kccnd-objs := udp_server.o	

else

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD:= $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

.PHONY: clean

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c