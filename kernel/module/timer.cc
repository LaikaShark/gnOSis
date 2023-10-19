#include <system.h>

// The Timer Tick
unsigned long long tick = 0;

unsigned long long get_timer()
{
  return tick;
}

//Prints the time
void timer_callback(registers_t regs)
{
  tick++;
  if(tick == 0)
    printj("DING\n");
}
void dead_timer() {}

void set_timer(u32int freq, void (f)(registers_t))
{
  if(freq == 0)
  {
    return;
  }
 // Raise the Interrupt to Initialise the Clock
  asm volatile("sti");

  // Register our Timer , with the address of the fuction to be called when
  // ever the Interrupt is raised
  register_interrupt_handler(IRQ0 , f);

  // We have to send a value to the PIT , which is a divisor which is used to divide the
  // Frequency (1193180 HZ) to get the desired frequency
  u32int divisor = 1193180 / freq;

  // Send to the command byte to the PIT so set it to repeat mode
  io::outb(0x43 , 0x36);

  // Divisor has to be split into two bytes , upper half and lower half
  u8int low = (u8int)(divisor & 0xFF);
  u8int high = (u8int)((divisor >> 8) & 0x0FF);

  // Send the Divisor
  io::outb(0x40 , low);
  io::outb(0x40 , high);
}
//Initialises the PIT and Sets the First IRQ to point to the Clock
//pass 0 to disable
void init_timer(u32int freq)
{
  printj("Initializing Timer......");
  if(freq == 0)
  {
    cprintj("DISABLED\n",RED,WHITE);
	return;
  }
  // Raise the Interrupt to Initialise the Clock
  asm volatile("sti");

  // Register our Timer , with the address of the fuction to be called when
  // ever the Interrupt is raised
  register_interrupt_handler(IRQ0 , &timer_callback);

  // We have to send a value to the PIT , which is a divisor which is used to divide the
  // Frequency (1193180 HZ) to get the desired frequency
  u32int divisor = 1193180 / freq;

  // Send to the command byte to the PIT so set it to repeat mode
  io::outb(0x43 , 0x36);

  // Divisor has to be split into two bytes , upper half and lower half
  u8int low = (u8int)(divisor & 0xFF);
  u8int high = (u8int)((divisor >> 8) & 0x0FF);

  // Send the Divisor
  io::outb(0x40 , low);
  io::outb(0x40 , high);
  printj("DONE\n");
}
