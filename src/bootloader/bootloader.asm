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
; FAT12 header
;
    jmp short start
    nop

bdb_oem:                    db 'MSWIN4.1'   ; 8 bytes
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluster:    db 1
bdb_reserved_sectors:       dw 1
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0E0h
bdb_total_sectors:          dw 2880     ; 2880 * 512 == 1.44MB 
bdb_media_descriptor_type:  db 0F0h     ; 3.5" floppy disk
bdb_sectors_per_fat:        dw 9
bdb_sectors_per_track:      dw 18
bdb_heads:                  dw 2
bdb_hidden_sectors:         dd 0
bdb_large_sector_count:     dd 0

; extended boot record
ebr_drive_number:           db 0    ; 0 means floppy
                            db 0    ; reserved
ebr_signature:              db 29h
ebr_volume_id:              db 11h, 22h, 33h, 44h   ; seriel number (value doesn't matter)
ebr_volume_label:           db 'BMOS       '    ; 11 bytes, space-padded
ebr_system_id:              db 'FAT12   '       ; 8 bytes, space-padded


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

    ; read something from floppy disk
    mov si, msg_disk_read
    call puts
    mov [ebr_drive_number], dl    ; BIOS should set drive number in dl
    mov ax, 1   ; 2nd sector of disk
    mov cl, 1   ; read l sector
    mov bx, 07E00h  ; store data after bootloader
    call disk_read

    ; display halting message
    mov si, msg_halt
    call puts
.halt:
    cli         ; prevents an interrupt from allowing us to escape from the 'hlt' instruction
    hlt         ; halts the processor
    jmp .halt   ; just in case we somehow escape from 'hlt'


;-------------------------------------------------------------------------------
; disk routines
;

;-------------------------------------------------------------------------------
;   SUBROUTINE lba_to_chs:
;       converts an LBA address to a CHS address
;   parameters:
;       ax: LBA address
;   returns:
;       cx [bits 6-15]: cylinder
;       dh:             head
;       cx [bits 0-5]:  sector number
lba_to_chs:
    push ax
    push dx

    ; sector = (LBA % Sectors_per_Track) + 1
    xor dx, dx                          ; initialize for division
    div word [bdb_sectors_per_track]    ; ax = LBA / Sectors_per_Track
                                        ; dx = LBA % Sectors_per_Track
    inc dx
    mov cx, dx                          ; cx = sector

    ; cylinder = (LBA / Sectors_per_Track) / Heads
    ; head =  (LBA / Sectors_per_Track) % Heads
    xor dx, dx                          ; initialize for another division
                                        ;   (ax already holds needed value)
    div word [bdb_heads]                ; ax = cylinder
                                        ; dx = head

    mov dh, dl                          ; dh = head
    mov ch, al                          ; ch = cylinder (low 8 bits)
    shl ah, 6
    or cl, ah                           ; 

    pop ax
    mov dl, al      ; restore dl
    pop ax
    ret 

;-------------------------------------------------------------------------------
;   SUBROUTINE disk_read:
;       reads sectors from a disk
;   parameters:
;       ax: LBA address
;   returns:
;       cl: number of sectors to read (up to 128)
;       dl: drive number
;       es:bx:  address to store data
disk_read:
    push ax
    push bx
    push cx
    push dx
    push di

    push cx             ; temporarily save cl (number of sectors to read)
    call lba_to_chs     ; compute CHS
    pop ax              ; al = number of sectors to read
    mov ah, 02h         ; interrupt function 02
    mov di, 3           ; retry count
.retry
    pusha               ; save all registers before BIOS messes them up
    stc                 ; some BIOSes don't set the carry flag
    int 13h
    jnc .done          ; carry is clear on success
    ; read failed
    popa
    call disk_reset
    dec di
    test di, di
    jnz .retry
 .fail:
    ; after repeated failed attempts 
    jmp floppy_error
.done:
    popa
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    ret

;-------------------------------------------------------------------------------
;   SUBROUTINE disk_reset
;   PARAMETERS:
;       dl: drive number
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret

;-------------------------------------------------------------------------------
; error handlers
;
floppy_error:
    mov si, msg_read_fail
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h         ; wait for keypress
    jmp 0FFFFh:0    ; jump to BIOS_start for reboot


;-------------------------------------------------------------------------------
; DATA SECTION
;
msg_startup:    db 'Starting BMOS...', ENDL, 0
msg_disk_read:  db 'Reading from disk...', ENDL, 0
msg_read_fail:  db 'Read from disk failed!', ENDL, 0
msg_halt:       db 'Halting processor!', ENDL, 0

; This file is the Boot Sector of our Boot Device (an emulated 3.5"
; 1.44MB floppy disk).  As such, it must assemble into exactly 512
; bytes.  We should zero-pad the rest this memory block, and add the
; AA55h signature to the end.

; pad the rest of the memory block with 00h
;   $  => address of current line
;   $$ => address of beginning of section
times 510-($-$$) db 0   ; zero-pad from current location to byte 510
dw 0AA55h               ; bytes 511/512
