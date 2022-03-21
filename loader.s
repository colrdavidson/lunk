[bits 64]
[org 0x18000]
[section .text]

start:
	jmp start

gdt_data:
	.null:	dq 0
	.code: 	dd 0xFFFF
			db 0
			dw 0xCF9A
			db 0
	.data: 	dd 0xFFFF
			db 0
			dw 0xCF92
			db 0
