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
  printj("                Version 0.3.03\n");
  printj("================[System Ready]================\n");

  char c = 0;
  char p[2];
  p[0] = 0;
  p[1] = 0;
  while(true)
  {
    c = keyboard_getchar();
    if(c != 0)
    {
  	  p[0] = c;
	  printj(p);
    }
  }

  return 1;
}
