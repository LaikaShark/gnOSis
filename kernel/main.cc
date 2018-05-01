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
	linein = keyboard_readline();
	if(str_eq(linein, (char*)"halt"))
	{
		clrscr();
		//TODO Add system cleanup here
		//     and move to deboot function
		printj("IT IS SAFE TO POWER OFF");
		return 1;
	}
	else if(str_eq((char*)"hello",linein))
	{
		for(int i=0; i<5; i++)
		{
			printj("HI!\n");
		}
	}
  }

  return 1;
}
