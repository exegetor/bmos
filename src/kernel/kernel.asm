org 0

; When the bootloader hands us control, we will be in the x86 16-bit real mode.
; We must tell the assembler to generate instructions of that type.
bits 16

; define macro for CR/LF
%define ENDL 0Dh, 0Ah


;-------------------------------------------------------------------------------
main:
.halt:
    ; display halting message
    mov si, msg_halt
    call puts
    cli         ; prevents an interrupt from allowing us to escape from the 'hlt' instruction
    hlt         ; halts the processor
    jmp .halt   ; just in case we somehow escape from 'hlt'

;-------------------------------------------------------------------------------
;   put string to screen
;   parameters:
;       ds:si points to string
puts:
    push si
    push ax
.loop:
    lodsb       ; al = next character
    or al, al   ; don't change al, but set zero flag if al is 0
    jz .done
    mov ah, 0Eh ; function 0E - pint in TTY mode
    mov bh, 0   ; page number 0.  bl not used in text mode
    int 10h     ; call BIOS putchar subroutine
    jmp .loop
.done:
    pop ax
    pop si
    ret

;-------------------------------------------------------------------------------
; data section
;
msg_halt:    db 'halting processor!', ENDL, 0
