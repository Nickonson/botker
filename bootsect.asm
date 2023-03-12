.code16
.att_syntax
_start:
    # seg addr init
    mov %cs, %ax        # seg addr of code to ax
    mov %ax, %ds        # savin as start of data  seg
    mov %ax, %ss        #      as start of stack seg
    mov $_start, %sp    # save addr stack as first instruct addr
    mov $0x0e, %ah      # indicate teletype mode for bios

    mov $0x48, %al
    int $0x10
    mov $0x65, %al
    int $0x10
    mov $0x6c, %al
    int $0x10
    mov $0x6c, %al
    int $0x10
    mov $0x6f, %al
    int $0x10

    call _inf_loop

_inf_loop:
    #hlt
    jmp _inf_loop
    .zero (512 -(. - _start) - 2)
    .byte 0x55, 0xAA
