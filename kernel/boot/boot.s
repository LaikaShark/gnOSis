;This is the boot strapper
;it is where the bios loads to boot from
;it also holds the multiboot header

MBOOT_PAGE_ALIGN 	equ 1<<0		;align kernel with page boundry
MBOOT_MEM_INFO	 	equ 1<<1		;Kernel memory info
MBOOT_HEADER_MAGIC 	equ 0x1BADB002	;Multiboot magin number
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO	;Fold the page and mem info into a single flag
MBOOT_CHECKSUM		equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32] 			;Setting our archetecture
[GLOBAL mboot]		;let our c parts access mboot
[EXTERN code]		;start of .text
[EXTERN bss]		;start of .bss
[EXTERN end]		;end of segment

mboot:
	dd MBOOT_HEADER_MAGIC	;GRUB looks for this to start the mboot header
	dd MBOOT_HEADER_FLAGS	;flags to tell GRUB how to load the kernel
	dd MBOOT_CHECKSUM		
	dd mboot				;where this descriptor lives
	dd code					;start of kernel code
	dd bss					;end of kernel's .data section 
	dd end					;end of kernel
	dd start				;kernel entry point

[GLOBAL start]				;kernel entry point
[EXTERN kmain]				;c entry point

start:
	push ebx				;load multiboot header location
	cli
	call kmain				;jump to c land
	jmp $					;spin to infinity
