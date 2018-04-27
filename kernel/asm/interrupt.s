;Interrupt wrapper and service function

;Macro to create a dummy Interrupt Service Routine (ISR)
;does not pass its own err code
%macro ISR_NOERRCODE 1
	global isr%1				
	isr%1:
		cli						;Disable interupts
		push byte 0				;push dummy err code
		push byte %1			;push int number
		jmp isr_common_stub		;goto common handler
%endmacro

;Actual service routine.
%macro ISR_ERRCODE 1
	global isr%1
	isr%1:
		cli
		push byte %1
		jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

extern isr_handler ;in isr.c

;this is the isr stub.
;it saves processor state, preps for kernel mode segmens, 
;calls the c handler and then resets the stack
isr_common_stub:
	pusha		;push all registers
	mov ax, ds	;low 16b of eax = ds
	push eax	;save data seg. descriptor

	mov ax, 0x10;load original data seg descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call isr_handler

	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa		;pop all reg
	add esp, 8	;drop the pushed error code and ISR num
	sti
	iret		;pops CS, EIP, EFLAGS, SS, and ESP and returns

;IRQ handlers for idt.h
;macro to create the interupt request functions
%macro IRQ 2
	[GLOBAL irq%1]	;name it param 1
	irq%1:
		cli
		push byte 0
		push byte %2
		jmp irq_common_stub
%endmacro

;IRQ Mappings
IRQ 0  , 32
IRQ 1  , 33
IRQ 2  , 34
IRQ 3  , 35
IRQ 4  , 36
IRQ 5  , 37
IRQ 6  , 38
IRQ 7  , 39
IRQ 8  , 40
IRQ 9  , 41
IRQ 10 , 42
IRQ 11 , 43
IRQ 12 , 44
IRQ 13 , 45
IRQ 14 , 46
IRQ 15 , 47


;Called when interrupt is raised
[EXTERN irq_handler]
;irq common stub, like the isr stub
irq_common_stub:
	pusha		;push all registers
	mov ax, ds	;low 16b of eax = ds
	push eax	;save data seg. descriptor

	mov ax, 0x10;load original data seg descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call irq_handler

	pop ebx		;reload original data seg descriptor
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa		;pop all
	add esp, 8 	;Drop error code and ISR num
	sti
	iret
