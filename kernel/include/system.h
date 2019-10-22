#ifndef SYSTEM_H_INCLUDED
#define  SYSTEM_H_INCLUDED

//Type standardizations
typedef unsigned int   u32int;
typedef          int   s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

//Headers for ease of use
#include "io.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "monitor.h"
#include "memory.h"
#include "timer.h"
#include "common.h"
#include "keyboard.h"
#include "string.h"
#include "forth.h"
#endif
