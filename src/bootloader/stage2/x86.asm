;-------------------------------------------------------------------------------
%macro x86_enter_real_mode 0
	[bits 32]
	; 1 - jump to 16-bit protected mode segment
	jmp word 18h:.pmode16
.pmode16:
	[bits 16]
	; 2 - disable protected mode bit in cr0
	mov eax, cr0
	and al, ~1
	mov cr0, eax
	; 3 - jump to real mode
	jmp word 00h:.rmode
.rmode:
	; 4 - setup segments
	mov ax, 0
	mov ds, ax
	mov ss, ax
	; 5 - enable interrupts
	sti
%endmacro

;-------------------------------------------------------------------------------
%macro x86_enter_protected_mode 0
	[bits 16]
	cli
	; 4 - set protection enable flag in CR0
	mov eax, cr0
	or al, 1
	mov cr0, eax
	; 5 - far jump into protected mode
	jmp dword 08h:.pmode32
.pmode32:
	; we are now in protected mode!
	[bits 32]
	; 6 - setup segment registers
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
%endmacro


;-------------------------------------------------------------------------------
; convert a linear address to segment:offset address
;	arg1 : linear address
;	arg2 : (out) target segment (e.g. es)
;	arg3 : target register to use (32-bit) (e.g., eax)
;	arg4 : lower half of arg3 (e.g. ax)
%macro linear_addr_to_seg_offset 4
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
;		(uint8_t drive,             // [bp +  8]
;		uint8_t* driveTypeOut,     // [bp + 12]
;		uint16_t* cylindersOut,    // [bp + 16]
;		uint16_t* headsOut,        // [bp + 20]
;		uint16_t* sectorsOut);     // [bp + 24]
; OUT
;	eax 0 => fail
;	eax 1 => success
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
	sbb eax, 0   ; or 0 if carry flag set

	; set output parameters

	; drive type (in bl)
	; [bp + 12] (on the stack) is a pointer to return drive_type.
	linear_addr_to_seg_offset [bp + 12], es, esi, si
	mov [es:si], bl ; drive_type is in bl 

	; cylinders (in cx[6-15])
	mov bl, ch      ; cylinders (lower 8 bits in ch)
	mov bh, cl      ; cylinders (upper bits 6/7 in cl)
	shr bh, 6
	inc bx
	linear_addr_to_seg_offset [bp + 16], es, esi, si
	mov [es:si], bx

	; sectors (in cl[0-5])
	xor ch, ch      ; sectors - low 5 bits in cl
	and cl, 00111111b
	linear_addr_to_seg_offset [bp + 24], es, esi, si
	mov [es:si], cx

	; heads
	mov cl, dh      ; heads - dh
	inc cx
	linear_addr_to_seg_offset [bp + 20], es, esi, si
	mov [es:si], cx

	; restore registers
	pop di
	pop esi
	pop bx
	pop es

	push eax    ; contains return value
	x86_enter_protected_mode
	[bits 32]
	pop eax

	; restore old call frame
	mov esp, ebp
	pop ebp
	ret


;-------------------------------------------------------------------------------
; bool _cdecl x86_Disk_Reset(uint8_t drive);
; IN    [bp + 8] : drive number
; OUT   eax : 0=fail, 1=success
global x86_disk_reset
x86_disk_reset:
	[bits 32]
	; make new call frame
	push ebp      ; save old call frame
	mov ebp, esp  ; initialize new call frame

	x86_enter_real_mode
	[bits 16]

	mov ah, 0
	mov dl, [bp + 8]    ; dl = drive
	stc         ; ensure we defult to "fail" state. (carry flag indictes failure)
	int 13h
	mov eax, 1  ; return 1 on success
	sbb eax, 0  ; or 0 if carry flag is set

	push eax
	x86_enter_protected_mode
	[bits 32]
	pop eax

	; restore old call frame
	mov esp, ebp
	pop ebp
	ret


;-------------------------------------------------------------------------------
; bool _cdecl x86_Disk_Read(uint8_t   drive,
;                           uint16_t  cylinder,
;                           uint16_t  head,
;                           uint16_t  sector,
;                           uint8_t   count,
;                           void far* dataOut);
; IN:	[bp +  8] : drive number
;		[bp + 12] : cylinder
;		[bp + 16] : head
;		[bp + 20] : sector
;		[bp + 24] : count
; OUT:	[bp + 28] : dataOut
;		eax : 0=fail, 1=success

global x86_disk_read
x86_disk_read:
	[bits 32]
	; make new call frame
	push ebp      ; save old call frame
	mov ebp, esp  ; initialize new call frame

	x86_enter_real_mode
	[bits 16]

	; save modified regs
	push ebx
	push es

	; set up args
	mov dl, [bp + 8]   ; dl = drive

	mov ch, [bp + 12]  ; ch - cylinder (lower 8 bits)
	mov cl, [bp + 13]  ; cl - cylinder to bits 6-7
	shl cl, 6          ;      (XX00:0000)
	mov al, [bp + 20]  ; al = low 8 bits of sector
	and al, 00111111b
	or cl, al          ; cl = sector to bits 0-5

	mov dh, [bp + 16]  ; dh = head
	mov al, [bp + 24]  ; al = count

	; set up for int13
	mov ah, 02h      ; function #2 
	linear_addr_to_seg_offset [bp + 28], es, ebx, bx    ; data will be sent to [es:bx]
	stc         ; ensure we defult to "fail" state. (carry flag indictes failure)
	int 13h
	mov eax, 1   ; return 1 on success
	sbb eax, 0   ; or 0 if carry flag set

	; restore regs
	pop es
	pop ebx

	push eax
	x86_enter_protected_mode
	[bits 32]
	pop eax

	; restore old call frame
	mov esp, ebp
	pop ebp
	ret
