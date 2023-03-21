.code16
.att_syntax
.globl _start
.text
_start:
    # seg addr init
    mov %cs, %ax        # seg addr of code to ax
    mov %ax, %ds        # savin as start of data  seg
    mov %ax, %ss        #       as start of stack seg
    mov $_start, %sp    # save addr stack as first instruct addr

    mov $0x1000, %bx
	mov %bx, %es
	mov $0x0, %bx
	mov $0, %ch         # cylinder numb
	mov $0, %dh         # head numb
	mov $1, %cl         # sector numb
	mov $1, %dl         # disk numb

    mov $0x02, %ah      # read from floppy to memory func
	mov $0x19, %al      # sectors amount 
	int $0x13

    # clearing the consol
    mov $0x03, %ax      # specific func for that
    int $0x10

    # moving back the corette
    mov $0x2, %ah
    xor %bh, %bh
    xor %dl, %dl
    int $0x10

    # int disable
    cli
    # loading size & addr of descr table
    lgdt gdt_info

    # turnin on adr line A20
    inb $0x92, %al
    orb $2, %al
    outb %al, $0x92

    # bit Pe of CR0 = 1
    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0
    
    # long jump for right info loading in cs, cant do that straight
    ljmp $0x8, $protected_mode

# describin global descriptors table
# data about GDT table
gdt:
    # null descr
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    # code seg : base = 0, size = 4 GB, P = 1, DPL = 0, S = 1 (usr)
    #            Type = 1 (code), Access = 00A, G = 1, B = 32 bit 
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
    # data seg : base = 0, size = 4 GB, P = 1, DPL = 0, S = 1 (usr)
    #            Type = 0 (data), Access = 0W0, G = 1, B = 32 bit
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info:
    .word gdt_info - gdt    # size : 2 byte
    .word gdt, 0            # 32 bit phys table addr


.code32
protected_mode:
    # loading seg selectors for stack and data in regs
    movw $0x10, %ax     # using descr №2 in GDT
    movw %ax, %es
    movw %ax, %ds
    movw %ax, %ss
    # passing work to loaded kernel
    call 0x10000

    .zero (512 -(. - _start) - 2)
    .byte 0x55, 0xAA
