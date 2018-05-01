#include <system.h>

bool str_eq(char* a, char* b)
{
	int i = 0;
	if(a[i] == '\0' && b[i] != '\0')
	{
		return false;
	}
	while(a[i] != '\0')
	{
		if(a[i] != b[i])
		{
			return false;
		}
	i++;
	}
	return true;
}
