#include <system.h>


// Declared as a C function so as to remove name mangling and call it from the assembly file
extern "C" int kmain(struct multiboot *mboot_ptr);

// The Main Function
int kmain(struct multiboot *mboot_ptr)
{

  clrscr();
  boot_text();
 
 // Initialise the Descritor Tables
  init_desc_tables();


  // Reenable Interrupts
  asm volatile("sti");

  // All device Intialisations Go Here
  init_keyboard_driver();
  init_funclib();
  init_timer(0); //disable unused timer
  printj("Color test:");
  for(int i=0; i < 16; i++)
  {
  	cprintj(" ",i,WHITE);
  }

  printj("\n                Version 0.3.03\n");
  printj("================[System Ready]================\n");

  char* linein;
  while(true)
  {
  	putch('>');
	linein = keyboard_readline();
	if(str_eq(get_split(linein, ' ', 1), (char*)"halt"))
	{
		clrscr();
		//TODO Add system cleanup here
		//     and move to deboot function
		printj("IT IS SAFE TO POWER OFF");
		return 1;
	}
	if(str_eq(get_split(linein, ' ', 1), (char*)"clear"))
	{
		clrscr();
	}
	else if(str_eq(get_split(linein, ' ', 1), (char*)"func"))
	{
		if(count_splits(linein, ' ') == 2)
		{
			call_function(str_to_int(get_split(linein, ' ', 2)));
		}
		else
		{
			printj("Usage: func [int]\n");
		}
	}
	else if(str_eq(linein, (char*)"test"))
	{
		cprintj("TESTING EXPERIMENTAL FUNCTIONALITY\n", BLACK, RED);
		printj(get_split(keyboard_readline(),' ',3));
		putch('\n');
	}
  }

  return 1;
}
