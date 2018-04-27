;defines out idt_flush() function
;sets up out interupt description table

[GLOBAL idt_flush]
idt_flush:
	mov eax, [esp+4]	;get the pointer param
	lidt [eax]			;load idt, its one op, not hard
	ret					;Bye!
