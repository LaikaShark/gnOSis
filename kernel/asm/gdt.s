;This holds our gdt_flush() func
;it sets up the global discriptor table

[GLOBAL gdt_flush]		;make it a global so we can call it from c
gdt_flush:				
	mov eax, [esp+4] 	;param 1: gtd pointer
	lgdt[eax]			;load it

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush		;0x08 is our code segment
.flush:
	ret
