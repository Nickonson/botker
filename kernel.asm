.code32
.att_syntax

.text
_start:
    # saves video buffer addr in %edi
    mov $0xb8000, %edi
    # placin str begin in %esi
    mov $str_hello, %esi
    call video_puts

_inf_loop:
    hlt
    jmp _inf_loop

video_puts:
    # after end %edi have adr at which u can continue outputin
    mov 0(%esi), %al
    test %al, %al
    jz video_puts_end

    # colour lightgrey 07 is default 0x2A is green
    mov $0x07, %ah
    mov %al, 0(%edi)
    mov %ah, 1(%edi)

    add $2, %edi
    add $1, %esi
    jmp video_puts

video_puts_end:
    ret

str_hello:
    .asciz "Welcome to DistrOS (asm edition)!"
