
.text
	.globl _start
	
	.code16

#initial
_start:
	movw $0x9000, %ax
	movw %ax, %ss
	xorw %sp, %sp

	movw $0x7C0,%dx
	movw %dx,%ds
	
	leaw msg - 0x1000,%si
	movw $len, %cx

# print string
1:	lodsb
	movb $0x0E,%ah
	int $0x10
	loop 1b
	

	movb $'0,%al
	movb $0x0E,%ah
	int $0x10

	movb $'x,%al
	int $0x10

#memory
	movw   $0xE801,%ax
        int    $0x15
	
	addw $0x400,%ax
	movw %ax,%cx
	movw %bx,%dx

	
	shr $8,%ax
	call print
	
	movw %cx,%ax
	andw $0x00FF,%ax
	call print

	


1:	jmp 1b


print:	pushw %dx
	movb %al, %dl
	shrb $4, %al
	cmpb $10, %al
	jge 1f
	addb $0x30, %al
	jmp 2f
1:	addb $55, %al		
2:	movb $0x0E, %ah
	movw $0x07, %bx
	int $0x10

	movb %dl, %al
	andb $0x0f, %al
	cmpb $10, %al
	jge 1f
	addb $0x30, %al
	jmp 2f
1:	addb $55, %al		
2:	movb $0x0E, %ah
	movw $0x07, %bx
	int $0x10
	popw %dx
	ret
	
	msg: .ascii "MemOS: Welcome *** System Memory is:"
	.set len, . - msg


	.org 0x1FE

	.byte 0x55
	.byte 0xAA
