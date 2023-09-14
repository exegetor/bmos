;-------------------------------------------------------------------------------
%macro x86_enter_real_mode 0
    [bits 32]
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
    [bits 16]
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
; converts a linear address to  segment:offset address
; arg1 : linear address
; arg2 : (out) target segment (e.g. es)
; arg3 : target register to use (32-bit) (e.g., eax)
; arg4 : lower half of arg3 (e.g. ax)
%macro linear_address_to_seg_offset 4
    mov %3, %1  ; store linear address in a register
    shr %3, 4   ; divide by 16, so register now contains the segment
    mov %2, %4  ; store segment in a different register
    mov %3, %1  ; reload linear address in 1st register
    and %3, 0Fh ; throw way all but the low 4 bits, so register now holds offset
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
;bool _cdecl x86_Disk_GetDriveParams
;       (uint8_t drive,             // [bp +  8]
;        uint8_t* driveTypeOut,     // [bp + 12]
;        uint16_t* cylindersOut,    // [bp + 16]
;        uint16_t* headsOut,        // [bp + 20]
;        uint16_t* sectorsOut);     // [bp + 24]
; OUT
;   eax 0 => fail
;   eax 1 => success
global x86_disk_get_drive_params
x86_disk_get_drive_params:
    [bits 32]
    ; make new call frame
    push ebp        ; save old call frame
    mov ebp, esp    ; initialize new call frame
    x86_enter_real_mode
    [bits 16]
    ; save registers
    push es
    push bx
    push esi
    push di

    ; call int13h
    mov dl, [bp + 8]    ; dl = drive ID
    mov ah, 08h
    mov di, 0           ; es:di = 0000:0000 to account for some BIOS bugs
    mov es, di
    stc                 ; ensure carry flag is set (indicates failure)
    int 13h
    mov eax, 1   ; return 1 on success
    sbb eax, 0   ; return 0 if carry flag set

    ; set output parameters

    ; drive type (in bl)
    ; [bp + 12] (on the stack) is a pointer to return drive_type.
    ;mov si, [bp + 6] <- old real-mode version
    linear_address_to_seg_offset [bp + 12], es, esi, si
    mov [es:si], bl ; drive_type is in bl 

    ; cylinders (in cx[6-15])
    mov bl, ch      ; cylinders (lower 8 bits in ch)
    mov bh, cl      ; cylinders (upper bits 6/7 in cl)
    shr bh, 6
    inc bx
    ;mov si, [bp + 8] <- old real-mode version
    linear_address_to_seg_offset [bp + 16], es, esi, si
    mov [es:si], bx

    ; sectors (in cl[0-5])
    xor ch, ch      ; sectors - low 5 bits in cl
    and cl, 00111111b
    ;mov si, [bp + 12] <- old real-mode version
    linear_address_to_seg_offset [bp + 24], es, esi, si
    mov [es:si], cx

    ; heads
    mov cl, dh      ; heads - dh
    inc cx
    ;mov si, [bp + 10] <- old real-mode vesion
    linear_address_to_seg_offset [bp + 20], es, esi, si
    mov [es:si], cx

    ; restore registers
    pop di
    pop esi
    pop bx
    pop es

    push eax    ; contains return value
    x86_enter_protected_mode ; (uses eax)
    [bits 32]
    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

;-------------------------------------------------------------------------------
; DELETE ME
; function for testing a temporary switch to real mode.  call from c.
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

