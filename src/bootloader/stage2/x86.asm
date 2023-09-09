bits 16

section _TEXT class=code

;-------------------------------------------------------------------------------
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOut, uint32_t *remainderOut);
;
global _x86_div64_32
_x86_div64_32:
    ; make new call frame
    push bp         ; save old call frame
    mov bp, sp      ; initialize new call frame
    push bx         ; required by _cdecl

    ; divide upper 32 bits
    xor edx, edx
    mov eax, [bp + 8]   ; eax = upper 32 bits of dividend
    mov ecx, [bp + 12]  ; ecx = divisor
    div ecx

    ; store upper 32 bits of quotient
    mov bx, [bp + 16]
    mov [bx + 4], eax

    ; divide lower 32 bits
    mov eax, [bp + 4]   ; eax = low 32 bits of dividend
                        ; edx = old remainder
    div ecx

    ; store result
    mov [bx], eax
    mov bx, [bp + 18]
    mov [bx], edx

     ; restore value per _cdecl rules
    pop bx
    ; restore old call frame
    mov sp, bp
    pop bp
    ret

;-------------------------------------------------------------------------------
; int 10h ah=0Eh
; args: character, page
;
global _x86_video_write_char_teletype ;export so we can call this from c
_x86_video_write_char_teletype:
    ; make new call frame
    push bp         ; save old call frame
    mov bp, sp      ; initialize new call frame
    push bx         ; required by _cdecl
    ; [bp + 0] = old call frame
    ; [bp + 2] = return address (small memory model -> 2 bytes);
    ; [bp + 4] = first argument (character)
    ; [bp + 6] = second argument (page)
    ; note: bytes are converted to words (you can't push a single byte on the stack
    mov ah, 0Eh
    mov al, [bp+4]
    mov bh, [bp+6]
    int 10h
    ; restore value per _cdecl rules
    pop bx
    ; restore old call frame
    mov sp, bp
    pop bp
    ret
