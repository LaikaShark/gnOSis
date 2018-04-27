#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <system.h>

//Copy set no of bytes from src to dest
void *memcopy(void *dest, const void *src, int count);

//Set 'count' bytes in 'dest' to 'val'.*/
void *memset(void *dest, char val, int count);

//same as above but with a word
u16int *memsetw(u16int *dest, u16int val, int count);


#endif
