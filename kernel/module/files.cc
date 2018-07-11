//this is the dreamer file system. It is a volotile system by design.
#include <system.h>

file FT[MAX_FILES];
char filebuff[MAX_FILE_LEN];

bool init_FT()
{
	for(int i=0; i< MAX_FILES; i++)
	{
		FT[i].free = true;
		for(int l=0;l<10;l++){FT[i].name[l]=0;}  
		for(int k=0;k<MAX_FILE_LEN;k++){FT[i].data[k] = 0;}
	}
	return true;
}

bool new_file(char * name)
{
	for(int i=0; i<MAX_FILES;i++)
	{
		if(FT[i].free == true)
		{
			FT[i].free = false;
			str_cpy(FT[i].name, name);
			return true;
		}
		if(str_eq(FT[i].name,name))
		{
			return false;
		}
	}
	return false;
}

bool del_file(char * name)
{
	for(int i=0; i<MAX_FILES; i++)
	{
		if(str_eq(name,FT[i].name))
		{
			FT[i].free = true;
			for(int l=0;l<10;l++){FT[i].name[l]=0;}  
			for(int k=0;k<MAX_FILE_LEN;k++){FT[i].data[k] = 0;}
			return true;
		}
	}
	return false;	
}
bool open_file(char * name)
{
	for(int i=0; i<MAX_FILES; i++)
	{
		if(str_eq(name,FT[i].name))
		{
			for(int k=0;k<MAX_FILE_LEN;k++)
			{
				filebuff[k] = FT[i].data[k];
			}
			return true;
		}
	}
	return false;
	
}
bool write_file(char* name)
{
	for(int i=0; i<MAX_FILES; i++)
	{
		if(str_eq(name,FT[i].name))
		{
			for(int k=0;k<MAX_FILE_LEN;k++)
			{
				FT[i].data[k] = filebuff[k];
			}
			return true;
		}
	}
	return false;	
}
void list_files()
{
	int free = MAX_FILES;
	for(int i=0; i<MAX_FILES;i++)
	{
		if(!FT[i].free)
		{
			print_dec(i);
			printj(": ");
			printj(FT[i].name);
			printj("\n");
			free --;
		}
	}
	printj("FREE: ");
	print_dec(free);
	printj("\n");	
}

