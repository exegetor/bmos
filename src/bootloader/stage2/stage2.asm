bits 16

section .entry

extern __bss_start
extern __end

extern start

global entry
entry:
	; we are going to switch to 32-bit protected mode
	; right away.  we can switch back and forth as
	; needed to access BIOS interrupts.

	mov [g_boot_drive], dl  ; save boot drive

	cli ; cannot process interrupts setting up segments and stack
	; set up stack so ss = ds
	mov ax, ds
	mov ss, ax
	mov sp, 0FFF0h
	mov bp, sp

	; switch to protected mode
	; see Intel Processor Manual, volume 3, section 9.9.l
	; https://pdos.csail.mit.edu/6.828/2008/readings/ia32/IA32-3A.pdf
	; and OSdev wiki
	; https://wiki.osdev.org/Protected_mode

	                    ; step 1:   disable interrupts (already disabled)
	call enable_A20     ; step 2:   enable the A20 gate
	call load_GDT       ; step 3:   load GDT

	mov eax, cr0
	or ax, CR0_protected_mode_bit
	mov cr0, eax
	jmp dword 08h:.pmode_start  ; far jump into GDT entry #1
	                            ; (this only advances to the next instruction)
.pmode_start:
	; now in protected mode
	[bits 32]
	; set up segment registers to a 32-bit data segment
	mov ax, g_GDT2-g_GDT       ; = 0x10
	mov ds, ax
	mov ss, ax

	; clear bss (uninitialized data)
	mov edi, __bss_start
	mov ecx, __end
	sub ecx, edi
	mov al, 0
	cld
	rep stosb

	; now we switch to c-code

	; send g_boot_drive as argument to start() function
	xor edx, edx
	mov dl, [g_boot_drive]
	push dx
	call start
	; start() should be an eternal loop,
	; but just in case it happens to return,
	; we'll do an eternal loop here...
.halt:
	jmp .halt


;-------------------------------------------------------------------------------
; setting the A20 gate high allows access to memory above 1MB
;
enable_A20:
	[bits 16]
	; disable keyboard
	call kbd_ctrl_wait_input        ; returns when controller is ready to receive data
	mov al, kbd_disable
	out kbd_controller_command_port, al
	call kbd_ctrl_wait_input        ; wait for controller to digest command

	; read control output port (i.e. flush output buffer)
	mov al, kbd_read_output
	out kbd_controller_command_port, al
	call kbd_ctrl_wait_output       ; returns when there is data to be read
	in al, kbd_controller_data_port
	push eax
	call kbd_ctrl_wait_input

	; write control output port
	mov al, kbd_write_output
	out kbd_controller_command_port, al
	call kbd_ctrl_wait_input
	pop eax
	or al, A20_bit
	out kbd_controller_data_port, al
	call kbd_ctrl_wait_input

	; enable keyboard
	mov al, kbd_enable
	out kbd_controller_command_port, al
	call kbd_ctrl_wait_input
	ret


;-------------------------------------------------------------------------------
; Returns when the keyboard controller is ready to receive data.
; This allows the slow chip time to digest a command before sending another.
kbd_ctrl_wait_input:
	[bits 16]
	; wait until status bit 2 (input buffer) is 0
	in al, kbd_controller_command_port  ; read status byte
	test al, kbd_ctrl_in_buf_bit        ; if bit 2 is low, we're ready to write
	jnz kbd_ctrl_wait_input
	ret


;-------------------------------------------------------------------------------
; This returns when there is data in the output buffer.
; Call this before attempting to read data from the controller port.
kbd_ctrl_wait_output:
	[bits 16]
	in al, kbd_controller_command_port  ; read status byte
	test al, kbd_ctrl_out_buf_bit       ; if bit 1 is high, we're ready to read
	jz kbd_ctrl_wait_output
	ret


;-------------------------------------------------------------------------------
load_GDT:
	[bits 16]
	lgdt [g_GDT_descriptor]
	ret


;-------------------------------------------------------------------------------
g_GDT:
g_GDT0:    ; entry 0 is the NULL descriptor
	dq 0

g_GDT1:    ; entry 1 is a 32 bit code segment
	dw 0FFFFh       ; limit[0-15]
	dw 0            ; base address[0-15]
	db 0            ; base address[16-23]
	db 10011010b    ; access flags
	                ;   present / ring 0
	                ;   code seg / executable / direction 0 / readable
	db 11001111b    ; more flags
	                ;   4KB page granularity / 32-bit protected mode / reserved / reserved
	                ;   limit[16-19]
	db 0            ; base[24-31]

g_GDT2:    ; entry 2 is a 32 bit data segment
	dw 0FFFFh       ; limit[0-15]
	dw 0            ; base address[0-15]
	db 0            ; base address[16-23]
	db 10010010b    ; access flags
	                ;   present / ring 0
	                ;   data seg / non-executable / direction 0 / writable
	db 11001111b    ; more flags
	                ;   4KB page granularity / 32-bit protected mode / reserved / reserved
	                ;   limit[16-19]
	db 0            ; base[24-31]

g_GDT3:    ; entry 3 is a 16 bit code segment
	dw 0FFFFh       ; limit[0-15]
	dw 0            ; base address[0-15]
	db 0            ; base address[16-23]
	db 10011010b    ; access flags
	                ;   present / ring 0
	                ;   code seg / executable / direction 0 / readable
	db 00001111b    ; more flags
	                ;   1 byte page granularity / 16-bit protected mode / reserved / reserved
	                ;   limit[16-19]
	db 0            ; base[24-31]

g_GDT4:    ; entry 4 is a 16 bit data segment
	dw 0FFFFh       ; limit[0-15]
	dw 0            ; base address[0-15]
	db 0            ; base address[16-23]
	db 10010010b    ; access flags
	                ;   present / ring 0
	                ;   data seg / non-executable / direction 0 / writable
	db 00001111b    ; more flags
	                ;   1 byte page granularity / 16-bit protected mode / reserved / reserved
	                ;   limit[16-19]
	db 0            ; base[24-31]


;-------------------------------------------------------------------------------
g_GDT_descriptor:
	dw g_GDT_descriptor - g_GDT - 1 ; size of GDT
	dd g_GDT                        ; address of GDT


;-------------------------------------------------------------------------------
g_boot_drive: db 0


;-------------------------------------------------------------------------------
g_Hello1:  db "Hello world from protected mode!", 0
g_Hello2:  db "Hello world from real mode!", 0

ScreenBuffer  equ 0B8000h


;-------------------------------------------------------------------------------
CR0_protected_mode_bit  equ 01h


;-------------------------------------------------------------------------------
; port numbers
kbd_controller_data_port     equ 60h
kbd_controller_command_port  equ 64h


;-------------------------------------------------------------------------------
; status register bits
kbd_ctrl_out_buf_bit  equ 01h
kbd_ctrl_in_buf_bit   equ 02h
A20_bit               equ 02h


;-------------------------------------------------------------------------------
; command codes
kbd_disable       equ 0ADh
kbd_enable        equ 0AEh
kbd_read_output   equ 0D0h
kbd_write_output  equ 0D1h
