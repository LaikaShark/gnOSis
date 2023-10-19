#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <system.h>

unsigned long long get_timer();
void timer_callback(registers_t regs);
//Initialises the Timer
void init_timer(u32int freq);
void set_timer(u32int freq, void (f)(registers_t));

#endif
