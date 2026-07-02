#ifndef KSTRING_H
#define KSTRING_H

#include "../../include/types.h"

void*   memset(void *dest, int val, size_t len);
void*   memcpy(void *dest, const void *src, size_t len);
size_t  strlen(const char *s);
int     strcmp(const char *a, const char *b);
int     strncmp(const char *a, const char *b, size_t n);
char*   strcpy(char *dest, const char *src);
char*   strncpy(char *dest, const char *src, size_t n);
char*   strcat(char *dest, const char *src);
void    itoa(int value, char *buf, int base);

#endif
