obj-m += keyboard_driver.o
PWD := $(shell pwd)
KDIR := $(shell uname -r)

all:
	make -C /lib/modules/$(KDIR)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KDIR)/build M=$(PWD) clean

msg:
	tail /var/log/syslog

