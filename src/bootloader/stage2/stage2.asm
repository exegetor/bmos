bits 16

section _ENTRY class=CODE

extern _cstart_

global entry

entry:
    ; set up stack so ss = ds
    cli             ; cannot process interrupts during stack setup
    mov ax, ds
    mov ss, ax
    mov sp, 0
    mov bp, sp
    sti

    ;expect boot drive in dl, send it as argument to cstart function
    xor dh, dh
    push dx
    call _cstart_   ; should never return

    ; should never happen
    cli
    hlt
