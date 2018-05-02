#ifndef common_h
#define common_h

#include <system.h>

#define OS_NAME "gnOSis"
#define OS_ARCH "x86"
#define USER	"osdev"
typedef void(*func_ptr)();
void init_desc_tables();
void boot_text();
void init_funclib();
void reg_function(void (*func)());
void hello();
void hello2();
void call_function(int in);
#endif
