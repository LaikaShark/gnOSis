#ifndef files_h
#define files_h
#include <system.h>
#define MAX_FILE_LEN 10
#define MAX_FILES 10
extern char filebuff[MAX_FILE_LEN];
struct file
{
    bool free;
    char name[10];
    char data[MAX_FILE_LEN];
};
bool init_FT();
bool new_file(char * name);
bool del_file(char * name);
bool open_file(char * name);
bool write_file(char * name);
void list_files();
#endif
