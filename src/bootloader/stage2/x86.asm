bits 16

section _TEXT class=code

;-------------------------------------------------------------------------------
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t* quotientOut, uint32_t* remainderOut);
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
    ; [bp + 0] - old call frame
    ; [bp + 2] - return address (small memory model -> 2 bytes)
    ; [bp + 4] - first argument (character)
    ; [bp + 6] - second argument (page)
    ; note: bytes are converted to words (you can't push a single byte on the stack
    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]
    int 10h
    ; restore value per _cdecl rules
    pop bx
    ; restore old call frame
    mov sp, bp
    pop bp
    ret

;-------------------------------------------------------------------------------
; bool _cdecl x86_Disk_Reset(uint8_t drive);
;
global _x86_Disk_Reset
_x86_Disk_Reset:

    ; make new call frame
    push bp     ; save old call frame
    mov bp, sp  ; initialize new call frame

    mov ah, 0
    mov dl, [bp + 4]
    stc
    int 13h     ; carry flag set on fail
    mov ax, 1   ; return 1 on success
    sbb ax, 0   ; return 0 if carry flag set

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

;-------------------------------------------------------------------------------
; bool _cdecl x86_Disk_Read(uint8_t   drive,    // [bp + 4]
;                           uint16_t  cylinder, // [bp + 6/7]
;                           uint16_t  head,     // [bp + 8]
;                           uint16_t  sector,   // [bp + 10]
;                           uint8_t   count,    // [bp + 12]
;                           void far* dataOut); // [bp + 14/16]
;
global _x86_Disk_Read
_x86_Disk_Read:

    ; make new call frame
    push bp     ; save old call frame
    mov bp, sp  ; initialize new call frame

    push bx
    push es

    ; set up args
    mov dl, [bp + 4]    ; dl = drive

;    mov cl, [bp + 7]    ; cl = bits 6/7 of cylinder
;    shl cl, 6           ;      xx000000
;    mov ch, [bp + 6]    ; ch = low 8 bits of cylinder
    mov ch, [bp + 6]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 7]    ; cl - cylinder to bits 6-7
    shl cl, 6           ;      (XX00:0000)

    mov al, [bp + 10]   ; al = low 8 bits of sector
    and al, 00111111b
    or cl, al           ; cl = sector to bits 0-5

    mov dh, [bp + 8]    ; dh = head

    mov al, [bp + 12]   ; al = count

    mov bx, [bp + 16]   ; es:bx = far pointer to data out
    mov es, bx
    mov bx, [bp + 14]

    mov ah, 02h
    stc
    int 13h     ; carry flag set on fail
    mov ax, 1   ; return 1 on success
    sbb ax, 0   ; return 0 if carry flag set

    pop es
    pop bx

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

;-------------------------------------------------------------------------------
;bool _cdecl x86_Disk_GetDriveParams
;       (uint8_t drive,             // [bp + 4]
;        uint8_t* driveTypeOut,     // [bp + 6]
;        uint16_t* cylindersOut,    // [bp + 8]
;        uint16_t* headsOut,        // [bp + 10]
;        uint16_t* sectorsOut);     // [bp + 12]
global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:

    ; make new call frame
    push bp     ; save old call frame
    mov bp, sp  ; initialize new call frame

    ; save registers
    push bx
    push si
    push es
    push di

    ; call int13h
    mov dl, [bp + 4]    ; dl = drive ID
    mov ah, 08h
    mov di, 0           ; es:di = 0000:0000 to account for some BIOS bugs
    mov es, di
    stc
    int 13h
    mov ax, 1   ; return 1 on success
    sbb ax, 0   ; return 0 if carry flag set

    ; out params
    mov si, [bp + 6]    ; drive type from bl
    mov [si], bl

    mov bl, ch      ; cylinters (lower 8 bits in ch)
    mov bh, cl      ; cylinders (upper bits 6/7 in cl)
    shr bh, 6
    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch      ; sectors = low 5 bits in cl
    and cl, 00111111b
    mov si, [bp + 12]
    mov [si], cx

    mov cl, dh      ; heads = dh
    mov si, [bp + 10]
    mov [si], cx

    ; restore registers
    pop di
    pop es
    pop si
    pop bx

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

;-------------------------------------------------------------------------------
;   SUBROUTINE U4D
;   unsigned 4 byte divide
;   IN: dx:ax   dividend
;       cx:bx   divisor
;   OUT: dx:ax  quotient
;        cx:bx  remainder
global __U4D
__U4D:
    ; put dx:ax into eax
    ; and clear edx
    shl edx, 16
    mov dx, ax
    mov eax, edx
    xor edx, edx

    ; put cx:bx into ecx
    shl ecx, 16
    mov cx, bx

    div ecx

    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    mov edx, eax
    shr edx, 16

    ret   

;-------------------------------------------------------------------------------
;   SUBROUTINE U4M
;   unsigned 4 byte multiply
;   IN: dx:ax   integer M1
;       cx:bx   integer M2
;   OUT: dx:ax  product
;        cx:bx  destroyed
global __U4M
__U4M:
    ; put M1 in eax
    shl edx, 16
    mov dx, ax
    mov eax, edx

    ; put M2 in ecx
    shl ecx, 16
    mov cx, bx

    mul ecx         ; result in edx:eax
    mov edx, eax    ; lower part will stay in ax
    shr edx, 16     ; shift upper part down to dx

    ret
