//Interrupt service routines
#include <system.h>

isr_t interrupt_handlers[256];

//called from asm when int is raised
extern "C" void isr_handler(registers_t reg)
{
	printj(" [EXCEPTION: ");
	print_dec(reg.int_no);
	printj("]\n");
}

//Handles IRQ raised by timer
extern "C" void irq_handler(registers_t reg)
{
	if(reg.int_no >= 40)
	{
		io::outb(PIC_SLAVE_COMMAND, 0x20);
	}
	
	io::outb(PIC_MASTER_COMMAND, 0x20);

	if(interrupt_handlers[reg.int_no] != 0)
	{
		isr_t handler = interrupt_handlers[reg.int_no];
		handler(reg);
	}
}

void register_interrupt_handler(u8int n, isr_t handler)
{
	interrupt_handlers[n] = handler;
}
