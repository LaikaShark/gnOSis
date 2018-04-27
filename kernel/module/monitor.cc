#include <system.h>

//Initialise the Driver
void monitor::init()
{
  cursor_x = cursor_y = 0;
  //VGA controller is at 0xB8000
  video_memory = (u16int *)0xB8000;
  isInitialised = true;
}

//ini coords
void monitor::seed(u8int x , u8int y)
{
  cursor_x = x;
  cursor_y = y;
}


//set cusor
void monitor::movecursor()
{
  // Calculate the Cursor Position
  u16int cursor_pos = cursor_y * 80 + cursor_x ;
  outb(0x3D4, 14);                //Alert VGA controller that we sent a high bit
  outb(0x3D5, cursor_pos >> 8);
  outb(0x3D4, 15);
  outb(0x3D5, cursor_pos);
}

// @ Task : Scroll the Terminal
void monitor::scroll()
{
  //The VGA Controller has 3 fields, bg color, fg color, character
  // Create a Blank with same color as fg and bg
  u8int attribute = ( BLACK << 4) | (WHITE & 0x0F);
  u8int blank = 0x20 | (attribute << 8);

  // A Standard Terminal size is 25 lines * 80 Coloumns
  if (cursor_y  >= 25)
    {
      // We are at the End of  the  Terminal so we have to move the entire text up one line
      int i;
      for(i = 0*80; i < 24*80; ++i)
        {
          video_memory[i] = video_memory[i+80];
        }

      // Now the Last Line is Left Blank , so we write 80 spaces using our blank character
      for (i = 24*80; i < 25*80; ++i)
        {
          video_memory[i] = blank;
        }

      // Set Y coordinate to last line
      cursor_y = 24;
    }
}


void monitor::putch(char ch)
{
  // Setup the bg and fg color
  u8int attribute =( BLACK << 4) |(WHITE & 0x0f);

  // Handle a backspace, by moving the cursor back one space
  if (ch == 0x08 && cursor_x)
    {
      cursor_x--;
    }

  // Handle a tab by increasing the cursor's X, but only to a point
  // where it is divisible by 8.
  else if (ch == 0x09)
    {
      cursor_x = (cursor_x+8) & ~(8-1);
    }

  // Handle carriage return
  else if (ch == '\r')
    {
      cursor_x = 0;
    }

  // Handle newline by moving cursor back to left and increasing the row
  else if (ch == '\n')
    {
      cursor_x = 0;
      cursor_y++;
    }

  // IF all the above text fails , print the Character
  if (ch >= ' ')
    {
      // Calculate the Address of the Cursor Position
      u16int *location = video_memory + (cursor_y * 80 + cursor_x);
      // Write the Bit into the cursor Postition
      *location = ch | (attribute << 8);
      cursor_x++;
    }

  // IF after all the printing we need to insert a new line
  if (cursor_x >= 80)
    {
      cursor_x = 0;
      cursor_y++;
    }

  // Scroll , or move the Cursor If Needed
  scroll();
  movecursor();
}

//Clear the Screen
void monitor::clear()
{
  // Let everthing Be Black
  u8int attribute = WHITE;
  u8int character = 0x20;

  // Pad Everyhting wth our attribute
  int i = 0;
  for (; i < 25*80; ++i)
    {
      video_memory[i] = character | (attribute << 8);
    }

  // Set Cursors to the Starting Position
  cursor_x = cursor_y = 0;
  movecursor();
}

//write a string to the console
void monitor::write(const char *c)
{
  int i = 0;

  while (c[i])
    {
      putch(c[i++]);
    }
}

//write dec to the console
void monitor::write_dec(int n)
{

  if (n == 0)
    {
      putch('0');
      return;
    }

  s32int acc = n;
  char c[32];
  int i = 0;
  while (acc > 0)
    {
      c[i] = '0' + acc%10;
      acc /= 10;
      i++;
    }
  c[i] = 0;

  char c2[32];
  c2[i--] = 0;
  int j = 0;
  while(i >= 0)
    {
      c2[i--] = c[j++];
    }
  write(c2);
}

//Print a Message on to the screen the simple way
// A monitor object is created globally
monitor mtr;
void printj(const char *s)
{
  if (mtr.isinit() == false)
    mtr.init();
  mtr.write(s);
}

void putch(char ch)
{
  if (mtr.isinit() == false)
    mtr.init();
  mtr.putch(ch);
}

//Print a Number on to the Screen
void print_dec(u32int n)
{
  if( mtr.isinit() == false)
    {
      mtr.init();
    }

  mtr.write_dec(n);
}

void clrscr()
{
  mtr.init();
  mtr.clear();
}
