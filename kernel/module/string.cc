#include <system.h>

bool str_eq(char* a, char* b)
{
	int i = 0;
	while(a[i] != '\0')
	{
		print_dec(a[i]);
		putch('\t');
		print_dec(b[i]);
		putch('\n');
		if(a[i] != b[i])
		{
			return false;
		}
	i++;
	}
	return true;
}
