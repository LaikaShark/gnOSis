#include <system.h>

//Zeroes out descriptors
void init_desc_tables()
{
  init_gdt();
  init_idt();
  memset(&interrupt_handlers, 0, sizeof(isr_t)*256);
}

//Boot text!
void boot_text()
{
  printj(" ######  ###    ##  ######  ####### ## #######\n");
  printj("##       ####   ## ##    ## ##      ## ##     \n");
  printj("##   ### ## ##  ## ##    ## ####### ## #######\n");
  printj("##    ## ##  ## ## ##    ##      ## ##      ##\n");
  printj(" ######  ##   ####  ######  ####### ## #######\n");
  printj("\n");
}

func_ptr func_lib[256] = {0};
void init_funclib()
{
	printj("Initializing Library....");
	reg_function(&hello);
	printj("DONE\n");
}
                 
void reg_function(void (*func)())
{
	int i = 0;
	while(func_lib[i] != 0)
	{
		i++;
	}
	func_lib[i] = func;
}

void hello()
{
	printj("Hello from the library\n");
}

void call_function(int in)
{
	func_lib[in]();
}
