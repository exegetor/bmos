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


;-------------------------------------------------------------------------------
; BEGIN CODE
;

;-------------------------------------------------------------------------------
main:
.halt:
    cli         ; prevents an interrupt from allowing us to escape from the 'hlt' instruction
    hlt         ; halts the processor
    jmp .halt   ; just in case we somehow escape from 'hlt'


;-------------------------------------------------------------------------------
; DATA SECTION
;
; This file is the Boot Sector of our Boot Device (an emulated 3.5"
; 1.44MB floppy disk).  As such, it must assemble into exactly 512
; bytes.  We should zero-pad the rest this memory block, and add the
; AA55h signature to the end.

; pad the rest of the memory block with 00h
;   $  => address of current line
;   $$ => address of beginning of section
times 510-($-$$) db 0   ; zero-pad from current location to byte 510
dw 0AA55h               ; bytes 511/512
