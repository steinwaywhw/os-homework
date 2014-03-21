
AS = as
LD = ld
VBM = vboxmanage
OC = objcopy
GCC = gcc
GCC_OPTION = -fno-builtin -fno-builtin-functions -fno-stack-protector -nostdlib -nodefaultlibs

all: boot

boot.vdi: boot
	$(VBM) convertfromraw $< $@ --format VDI --uuid c26987eb-1cfc-4969-9a10-e15de2b45a49

boot: boot.out
	$(OC) -O binary -j .text $< $@

boot.out: boot.o
	$(LD) -Ttext 0x7c00 -e _start -o $@ $^

boot.o:	boot.s
	$(AS) -o $@ $<

clean:
	rm boot boot.out boot.o boot.vdi


kentry.bin : kentry.o loader.o
	$(LD) -e loader --oformat elf32-i386 -o $@ $^

kentry.o : kentry.c
	$(GCC) $(GCC_OPTION) -o $@ -c $<

loader.o : loader.s
	$(AS) -o $@ $<

debug: bootd.out

bootd.out: bootd.o
	$(LD) -Ttext 0x0000 -e _start -o $@ $<

bootd.o: boot.s
	$(AS) -g -o $@ $<


num_to_str:
	gcc -c num_to_str.c
	ld -o num_to_str -Ttext 0x0 -e main num_to_str.o
	objcopy -R .note -R .comment -S -O binary num_to_str num_to_str.bin
