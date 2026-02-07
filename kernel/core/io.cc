//IO functions to talk to ports
#include <system.h>

//write byte to port
void io::outb(u16int port, u8int value)
{
	asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

//write word to port
void io::outw(u16int port, u16int value)
{
	asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

//read byte from port
u8int io::inb(u16int port)
{
	u8int ret;
	asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

//writes a word to port
u16int io::inw(u16int port)
{
	u16int ret;
	asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}
