	.global loader

	.set ALIGN,    1<<0                     # align loaded modules on page boundaries
    .set MEMINFO,  1<<1                     # provide memory map
    .set FLAGS,    ALIGN | MEMINFO          # this is the Multiboot 'flag' field
    .set MAGIC,    0x1BADB002               # 'magic number' lets bootloader find the header
    .set CHECKSUM, -(MAGIC + FLAGS)         # checksum required

    .align 4			#align 32bit boundary
    .long MAGIC 		#real header begins
    .long FLAGS
    .long CHECKSUM 

	
#MACHINE STATE ACCORDING TO SPEC
#EAX - Magic Number
#EBX - Address of the Multiboot info struct
#ESP - The OS image must create its own stack as soon as it needs one. 

	.set   STACKSIZE, 0x4000            # that is, 16k.
    .comm  stack, STACKSIZE           	# reserve 16k stack on a doubleword boundary

loader:	
	movl  $(stack + STACKSIZE), %esp

	pushl %ebx 							#pass arguments to c functions
	pushl %eax   						

	call  kentry

loop:
	hlt
	jmp	  loop

