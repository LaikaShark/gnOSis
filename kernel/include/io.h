#ifndef IO_INCLUDED
#define IO_INCLUDED

#include "system.h"

class io
{
	public:
		static void outb(u16int port, u8int val);
		static void outw(u16int port, u16int val);
		static u8int inb(u16int port);
		static u16int inw(u16int port);
};

#endif
