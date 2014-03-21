

obj-m += ramdisk.o
PWD := $(shell pwd)
KDIR := $(shell uname -r)
ramdisk-objs := fs.o fs_syscall.o ram_fs_kernel.o

all:
	make -C /lib/modules/$(KDIR)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KDIR)/build M=$(PWD) clean


user:
	gcc -g -o test_fs test_fs.c fs.c fs_syscall.c
kernel:
	gcc -g -o test_fs test_fs.c fs_lib.c
run:
	sudo insmod ./ramdisk.ko
	./test_fs
	sudo rmmod ramdisk
	#dmesg