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

int str_len(char* in)
{
	int i = 0;
	while(in[i] != '\0') {i++;}
	return i;
}
//string to int. returns 0 if fails
int str_to_int(char* s)
{
	int result = 0;
	int i = 0;
	if(s[0] == '-')
	{
		i++;
	}
	while(s[i] != '\0')
	{
		result *= 10;
		if(!('0' <= s[i] && s[i] <= '9') && s[0] != '-')
		{
			return 0;
		}
		if(s[0] == '-' && str_len(s) == 2)
		{
	 		return 0;	
		}
		switch(s[i])
		{
			case '0':
				break;
			case '1':
				result += 1;
				break;
			case '2':
				result += 2;
				break;
			case '3':
				result += 3;
				break;
			case '4':
				result += 4;
				break;
			case '5':
				result += 5;
				break;
			case '6':
				result += 6;
				break;
			case '7':
				result += 7;
				break;
			case '8':
				result += 8;
				break;
			case '9':
				result += 9;
				break;
		}
		i++;
	}
	if(s[0] == '-') 
	{
		result *= -1;
	}
	return result;
}

char** str_split(char* in, char delim, int* numtokens)
{
	
}
