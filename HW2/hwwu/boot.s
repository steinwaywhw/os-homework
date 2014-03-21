	.text
	.global _start
	.code16

	.set 	SEGMENT_ADDR, 	0x0000
	.set 	STACK_BASE, 	0x1000


	#.extern	e802				#call e820 (e820.s)

########################################################################
_start:
	movw	$SEGMENT_ADDR, %ax	#init ds, es and ss to 0x0000
#	movw	%ax, %cs 			#can't do this, or you will halt
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	

	xorw	%si, %si 			#clear si, di
	xorw	%di, %di 	

	movw	$STACK_BASE, %ax	#init bp, sp to 0x1000
	movw	%ax, %bp
	movw	%ax, %sp

#	movw	$0x07c0, %ax		#This is for use of LEA, instead of MOV
#	movw 	%ax, %ds 			#LEA will convert program counter into DS:OFFSET, which the offset = counter
#	movw	%ax, %es 			#See my blog steinwaywu.info for more information

	call	welcome
	call 	e820
	call 	print_mmard
	jmp		.


#	.align	4

########################################################################
welcome:
	pushl 	%eax
	pushl 	%ecx
	movw	$0x0000, %dx 		# 0 row, 0 column
	movl 	$msg_welcome, %eax
#	leal 	msg_welcome, %eax
	movl 	$len_welcome, %ecx
#	leal 	len_welcome, %ecx
	call 	print_string
	call 	newline
	popl	%ecx
	popl	%eax
	ret	

msg_welcome:	
	.ascii	"MemOS by Hanwen!"
	.set 	len_welcome, . - msg_welcome

########################################################################3

	.set	SMAP_MAGIC,	0x534d4150	#magic number
	.set	MMARD_SIZE, 20			#memory map address range descriptor size

	#RETURN: eax for counter, es:si for buffer

e820:
#	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%ebp
	pushl	%esp

	movl	$0x0000e820, %eax 	#eax = 0000e820
	xorl	%ebx, %ebx 			#ebx = 0 for start
	movl	$MMARD_SIZE, %ecx 	#ecx for address descriptor size, 20 or 24
	movl	$SMAP_MAGIC, %edx 	#edx for magic number
								#es is set by the caller, suppose to be zero
	movl	$buffer, %edi 		#edi is set to be the buffer
	xorw	%bp, %bp 			#bp cleared for counter		
	int 	$0x15

	jc		fail				#cf set on fail

	movl	$SMAP_MAGIC, %edx 	#prevent some BIOSes who clear it
	cmpl	%eax, %edx 			#eax should eq edx on success
	jne 	fail				#else, fail

loop:
	test	%ebx, %ebx 			#ebx & ebx = 0 if finished
	je 		finish 			

	incw	%bp				 	#increase counter
	addl	$MMARD_SIZE, %edi 	#increase edi

	movl	$0x0000e820, %eax 	#reset eax each time
	movl	$MMARD_SIZE, %ecx 	#reset ecx each time
	int 	$0x15 				#next call
	jmp 	loop				#loop until ebx = 0

finish:
	xorl	%eax, %eax 			#reset eax
 	movw 	%bp, %ax 			#copy counter to ax
 	movl 	$buffer, %esi 		#copy address to esi
 	clc 						#clear cf
 	popl 	%esp
 	popl 	%ebp
 	popl 	%edx
 	popl 	%ecx
 	popl 	%ebx

 	ret

fail:
	hlt

########################################################################
newline:
	pushl	%eax
	pushl 	%ebx
	pushl 	%ecx

	movb 	$0x03, %ah 			#read cursor position
	movb 	$0x00, %bh			#page number

	int 	$0x10 				#read cursor position

	addb   	$0x01, %dh			#dh contains current row, add 1 to move to next line
	xorb	%dl, %dl			#dl contains current column, reset
	movb	$0x02, %ah			#set cursor position

	int    	$0x10 				#set cursor position

	popl 	%ecx
	popl 	%ebx
	popl 	%eax

	ret

########################################################################
	#ecx for length, eax for address
print_string:
	pushl	%ebp
	pushl 	%eax
	pushl 	%ebx
	pushl	%ecx

	movl	%eax, %ebp			#es:ebp for string addr
								#ecx for length
	xorl	%eax, %eax							
	movb	$0x13, %ah			#print string
	movb	$0x01, %al 			#cursor moves with string
	movb	$0x00, %bh			#bh for page number
	movb	$0x0c, %bl			#bl for attributes
	int		$0x10 				#int 10

	popl 	%ecx
	popl 	%ebx
	popl 	%eax
	popl	%ebp
	ret	


########################################################################
	#print one digit (lower 4 bits of al) in hex
print_hex_digit:
	
	pushl	%eax
	pushl 	%ebx

	andb	$0x0f, %al  		#mask higher 4 bits
	addb 	$0x30, %al 			#convert 0~9 to '0'~'9'
	cmpb 	$0x39, %al			#compare with '9'
	jle		do_print_hex		#if <= '9', print. else, convert to 'A'
	addb 	$0x07, %al

do_print_hex:
	movb	$0x0e, %ah 			#display char, and move cursor
	movb	$0x00, %bh 			#page number
	movb	$0x0c, %bl 			#attributes

	int   	$0x10 				#print

	popl 	%ebx
	popl 	%eax 

	ret

########################################################################
	#esi for address, and esi = esi + 4 after call
print_32:
	pushl	%ecx
	movl	$4, %ecx			#loop counter
	addl	$3, %esi
do_print_32:
	movb 	(%esi), %al
	shrb	$4, %al
	call 	print_hex_digit

	movb	(%esi), %al
	call 	print_hex_digit

	decl	%esi
	loop 	do_print_32

	addl	$5, %esi
	popl 	%ecx
	ret

########################################################################
	#esi for address, and esi = esi + 8 after call
print_64:
	addl	$4, %esi
	call 	print_32
	addl	$-8, %esi
	call 	print_32
	addl 	$4, %esi
	ret

########################################################################
	#es:si for begin address, eax for count
print_mmard:
	pushl 	%eax
	pushl 	%esi
	pushl	%ecx

	call 	newline

entry_loop:
	pushl	%eax

do_print_base:
	movl	$str_base, %eax
	movl	$len_base, %ecx
	call 	print_string

	call 	print_64
								#I DON"T KNOW WHY, BUT THIS CURSOR WORKAROUND ONLY WORKS HERE, INSTEAD OF IN THE print_string().
	movb 	$0x03, %ah 			#read cursor position
	movb 	$0x00, %bh			#page number
	int 	$0x10 				#read cursor position

	
do_print_length:
	movl	$str_length, %eax
	movl	$len_length, %ecx
	call 	print_string

	call 	print_64
	movb 	$0x03, %ah 			#read cursor position
	movb 	$0x00, %bh			#page number
	int 	$0x10 				#read cursor position

	
do_print_type:
	movl	$str_type, %eax
	movl	$len_type, %ecx
	call 	print_string

	call 	print_32

	call 	newline
	popl	%eax
	decl 	%eax
	cmpl 	$0, %eax
	jge		entry_loop

	call 	newline

	addl	$-20, %esi
	movl 	$total, %eax
	movl 	%eax, %edi
	call 	compute_end_address

	movl	%edi, %eax
	movl	%eax, %esi
	call 	print_64

	popl	%ecx
	popl	%esi
	popl 	%eax

	ret

########################################################################
	#es:si for descriptor begin address
	#es:di for output qword
	#everything unchanged
compute_end_address:
#	pushl 	%eax
#	pushl 	%ebx
#	pushl 	%ecx
#	pushl	%edx

	movl 	(%esi), %eax
	movl 	4(%esi), %ebx
	movl 	8(%esi), %ecx
	movl 	12(%esi), %edx

	addl	%ecx, %eax
	adcl 	%edx, %ebx

	movl 	%eax, (%edi)
	movl 	%ebx, 4(%edi)

	ret

#	popl 	%edx
#	popl 	%ecx
#	popl	%ebx
#	popl 	%eax


########################################################################
str_base:
	.ascii	"BASE: "
	.set 	len_base, . - str_base
str_length:
	.ascii 	" LEN: "
	.set 	len_length, . - str_length
str_type:
	.ascii 	" TYPE: "
	.set 	len_type, . - str_type
str_total:
#	.ascii 	"Total "
#	.set   	len_total, . - str_total
str_unit:
#	.ascii  " Bytes"
#	.set 	len_unit, . - str_unit

########################################################################
	.org	0x01fe				#510 Byte
	.byte	0x55				
	.byte	0xaa

buffer:
	.fill 	20 * MMARD_SIZE, 1 
	.set	buffer_len, . - buffer

total:
	.word 	0x0
	.word 	0x0
	.word 	0x0
	.word 	0x0
