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
  init_timer(1); //disable unused timer
  init_ata();
  fs_init();
  printj("Color test:");
  for(int i=0; i < 16; i++)
  {
    cprintj(" ",i,WHITE);
  }

  printj("\n                Version 1.0.0");
  printj("\n================[System Ready]================\n");
  forth();

  clrscr();
  for(int i=0; i < 12; i++) {printj("\n");}
  printj("                          IT IS NOW SAFE TO POWER DOWN                          ");
  for(int i=0; i < 12; i++) {printj("\n");}
  return 1;
}
