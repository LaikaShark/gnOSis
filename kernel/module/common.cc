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
	reg_function(&hello2);
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

void hello2()
{
	printj("the function library is ");
	cprintj("expandable", BLACK, RED);
	printj(" up to 256 entries\n");
}

void call_function(int in)
{
	if(func_lib[in] == 0)
	{
		cprintj("NO FUNC FOUND\n", RED, WHITE);
		return;
	}
	cprintj("[Loading library function: ",BLACK,BROWN);
	cprint_dec(in,BLACK,BROWN);
	cprintj("]\n",BLACK,BROWN);
	func_lib[in]();
}
