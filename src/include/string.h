#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H

int          strcmp(const char *a, const char *b);
int          strncmp(const char *a, const char *b, unsigned int n);
unsigned int strlen(const char *s);
char        *strcpy(char *dst, const char *src);
void        *memset(void *dst, int val, unsigned int n);
void        *memcpy(void *dst, const void *src, unsigned int n);
int          atoi(const char *s);

#endif /* INCLUDE_STRING_H */
