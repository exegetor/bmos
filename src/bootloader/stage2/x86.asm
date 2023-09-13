;-------------------------------------------------------------------------------
%macro x86_enter_real_mode 0
    [bits 32]
;    cli     ; disable interrupts
    jmp word 18h:.pmode16
.pmode16:
    [bits 16]
    mov eax, cr0
    and al, ~1
    mov cr0, eax
    jmp word 00h:.rmode
.rmode:
    mov ax, 0
    mov ds, ax
    mov ss, ax
    sti             ; re-enable interrupts    
%endmacro

;-------------------------------------------------------------------------------
%macro x86_enter_protected_mode 0
;    [bits 16]
    cli
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp dword 08h:.pmode32
.pmode32:
    [bits 32]
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
%endmacro
    

;-------------------------------------------------------------------------------
global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

;-------------------------------------------------------------------------------
global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

;-------------------------------------------------------------------------------
global x86_realmode_putc
x86_realmode_putc:
    [bits 32]
    push ebp
    mov ebp, esp
    x86_enter_real_mode
    mov al, [bp + 8]
    mov ah, 0Eh
    int 10h
    x86_enter_protected_mode
    mov esp, ebp    ; restore stack pointer
    pop ebp
    ret

