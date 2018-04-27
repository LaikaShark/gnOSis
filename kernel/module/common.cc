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


                                               

