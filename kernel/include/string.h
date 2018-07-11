#ifndef STRING_H
#define STRING_H
void str_cpy(char* a, char* b);
bool str_eq(char* a, char* b);
int str_len(char* in);
int str_to_int(char* s);
int count_splits(char* s, char delim);
char* get_split(char* s, char delim, int split);
#endif
