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
ebr_volume_id:              db 11h, 22h, 33h, 44h   ; serial number (value doesn't matter)
ebr_volume_label:           db 'BMOS       '        ; 11 bytes, space-padded
ebr_system_id:              db 'FAT12   '           ; 8 bytes, space-padded


;-------------------------------------------------------------------------------
start:
    ; initialize segment registers
    mov ax, 0   ; (because we can't write directly to segment registers)
    mov ds, ax
    mov es, ax
    ; initialize stack address
    mov ss, ax
    mov sp, 7c00h

    ; some BIOSes will start us at 07c00:0000 instead of 0000:7c00
    ; Here we ensure we are at the later address by pushing the correct
    ; address to the stack and "retf" to that address.  (because only a
    ; JMP FAR can write to CS:IP)) 
    push es
    push word .after
    retf
.after:
    ; display startup message
    mov si, msg_startup
    call puts

    ; read Sector 02 from the floppy disk
    mov [ebr_drive_number], dl  ; BIOS should set drive number in dl
    mov ax, 1                   ; 2nd sector of disk
    mov cl, 1                   ; read l sector
    mov bx, 07E00h              ; store data after bootloader
    call disk_read

    ; read drive parameters (sectors per track and head count) via BIOS
    ; instead of relying on the data from the (unreliable?) formatted disk
    push es
    mov ah, 08h
    int 13h
    jc floppy_error
    pop es

    xor ch, ch
    and cl, 3Fh      ; cx = 0000:00xx
    mov [bdb_sectors_per_track], cx
    inc dh
    mov [bdb_heads], dh     ; head count

    ; read FAT root diectory...

    ; calculate LBA of root directory = reserved_sector_count +
    ;                                   fat_count * sectors_per_fat
    mov ax, [bdb_sectors_per_fat]
    xor bh, bh
    mov bl, [bdb_fat_count]
    mul bx 
    add ax, [bdb_reserved_sectors]
    push ax

    ; compute size of root directory = (32 bytes * number of entries) / bytes_per_sector
    xor dx, dx
    mov ax, [bdb_sectors_per_fat]
    shl ax, 5       ; ax *= 32
    div word [bdb_bytes_per_sector]

    test dx, dx
    jz read_root_dir
    add ax, 1   ; if remainder > 0, round up

 read_root_dir:
    mov cl, al                      ; number of sectors to read
    pop ax                          ; LBA of root directory
    mov dl, [ebr_drive_number]
    mov bx, buffer                  ; es:bx = buffer
    call disk_read

    xor bx, bx                      ; bx = directory entry counter
    mov di, buffer
.search_stage2:
    ; search for stage2.bin
    mov si, file_stage2_bin
    mov cx, 11                      ; compare up to 11 characters
    push di
    repe cmpsb                      ; compare bytes at ds:si with those at es:di
    pop di
    je .found_stage2
    add di, 32                      ; move on the the next directory entry
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_stage2
    jmp error_stage2_not_found

.found_stage2:
    ; di should have the address to the directory entry
    mov ax, [di+26] ; ax = first logical cluster field (offset 26)
    mov [stage2_cluster], ax

    ; load FAT from disk into memory
    mov ax, [bdb_reserved_sectors]
    mov bx, buffer
    mov cl, [bdb_sectors_per_fat]
    mov dl, [ebr_drive_number]
    call disk_read

    ; read stage2 and process FAT chain
    mov bx, STAGE2_LOAD_SEGMENT
    mov es, bx
    mov bx, STAGE2_LOAD_OFFSET

.load_stage2_loop:
    ; read next cluster
    mov ax, [stage2_cluster]
    ; shouldn't hardcode value (works for 1.44MB floppy only)
    add ax, 31      ; first cluster = (stage2_cluster - 2) * sectors_per_cluster + start_sector
                    ; start_sector = reserved + fats + root_directory_size = 1 + 18 + 14 = 33
    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]
    ; compute location of next cluster
    mov ax, [stage2_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx              ; ax = index of entry in FAT; dx = cluster mod 2

    mov si, buffer
    add si, ax
    mov ax, [ds:si]     ; read entry from FAT table at index ax

    or dx, dx
    jz .even
.odd:
    shr ax, 4
    jmp .next_cluster_after
.even:
    and ax, 0FFFh
.next_cluster_after:
    cmp ax, 0FF8h        ; marks end of chain
    jae .read_finish
    mov [stage2_cluster], ax
    jmp .load_stage2_loop

.read_finish:
    ; jump to our stage2
    mov dl, [ebr_drive_number]  ; boot device in dl
    mov ax, STAGE2_LOAD_SEGMENT ; set segment registers
    mov ds, ax
    mov es, ax
    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET


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
.retry:
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

error_stage2_not_found:
    mov si, msg_missing_stage2
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h         ; wait for keypress
    jmp 0FFFFh:0    ; jump to BIOS_start for reboot


;-------------------------------------------------------------------------------
; DATA SECTION
;

msg_startup:    db 'starting BMOS', ENDL, 0
msg_read_fail:  db 'read fail', ENDL, 0
msg_missing_stage2: db 'no stage2', ENDL, 0

file_stage2_bin db 'STAGE2  BIN'
stage2_cluster: dw 0

STAGE2_LOAD_SEGMENT equ 2000h
STAGE2_LOAD_OFFSET equ 0000h
; This file is the Boot Sector of our Boot Device (an emulated 3.5"
; 1.44MB floppy disk).  As such, it must assemble into exactly 512
; bytes.  We should zero-pad the rest of this memory block, and add
; the AA55h signature to the end.
;      $ => address of current line
;     $$ => address of beginning of section
times 510-($-$$) db 0   ; zero-pad from current location to byte 510
dw 0AA55h               ; bytes 511/512

buffer: