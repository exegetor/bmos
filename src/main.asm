; This file is the Boot Sector of our Boot Device (an emulated 3.5"
; 1.44MB floppy disk).
;
; The BIOS will check the boot sector for the signature (AA55h) at
; 00FE/00FF, and (if the signature matches) will load the boot sector
; into address 0000:7C00 (or maybe 07C0:0000) and begin executing the
; code located at that address.  We must specify that this code will
; go there.
org 7C00h

; When the BIOS hands us control, we will be in 16-bit real mode.
; We must tell the assembler to generate instructions of that type.
bits 16

; define macro for CR/LF
%define ENDL 0Dh, 0Ah

;-------------------------------------------------------------------------------
; BEGIN CODE
;

;-------------------------------------------------------------------------------
start:
    jmp main

;-------------------------------------------------------------------------------
;   SUBROUTINE PUTS: put string to screen
;   PARAMETERS:
;       ds:si points to string
puts:
    push si
    push ax
.loop:
    lodsb       ; al = next character
    or al, al   ; don't change al, but set zero flag if al is 0
    jz .done
    mov ah, 0Eh ; print in TTY mode
    mov bh, 0   ; page number 0.  (bl not used in text mode)
    int 10h     ; call BIOS putchar subroutine
    jmp .loop
.done:
    pop ax
    pop si
    ret


;-------------------------------------------------------------------------------
main:
    ; initialize segment registers
    mov ax, 0   ; (because we can't write directly to segment registers)
    mov ds, ax
    mov es, ax
    ; initialize stack address
    mov ss, ax
    mov sp, 7c00h

    ; display startup message
    mov si, msg_startup
    call puts

    ; display halting message
    mov si, msg_halt
    call puts
.halt:
    cli         ; prevents an interrupt from allowing us to escape from the 'hlt' instruction
    hlt         ; halts the processor
    jmp .halt   ; just in case we somehow escape from 'hlt'


;-------------------------------------------------------------------------------
; DATA SECTION
;

msg_startup: db 'starting BMOS...', ENDL, 0
msg_halt:    db 'halting processor!', ENDL, 0

; This file is the Boot Sector of our Boot Device (an emulated 3.5"
; 1.44MB floppy disk).  As such, it must assemble into exactly 512
; bytes.  We should zero-pad the rest this memory block, and add the
; AA55h signature to the end.

; pad the rest of the memory block with 00h
;   $  => address of current line
;   $$ => address of beginning of section
times 510-($-$$) db 0   ; zero-pad from current location to byte 510
dw 0AA55h               ; bytes 511/512
