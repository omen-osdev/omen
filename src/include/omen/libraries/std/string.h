#ifndef _STRING_H
#define _STRING_H
#define STR_MAX_SIZE 65536

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wreturn-type"

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/std/stdio.h>
#include <omen/libraries/std/stdbool.h>

typedef uint64_t size_t;

#ifndef __FUNCTION_NAME__
    #ifdef WIN32   //WINDOWS
        #define __FUNCTION_NAME__   __FUNCTION__  
    #else          //*NIX
        #define __FUNCTION_NAME__   __func__ 
    #endif
#endif

#define __UNDEFINED() (printf("[STRING.H ERROR] Undefined function: %s\n", __FUNCTION_NAME__))

void *memchr(const void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *strcat(char *dest, const char *src);
void *strncat(char *dest, const char *src, size_t n);
void *strchr(const char *s, int c);
int strcoll(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
size_t strcspn(const char *s, const char *reject);
char *strerror(int errnum);
char *strpbrk(const char *s, const char *accept);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
char *strstr(const char *haystack, const char *needle);
char *strtok(char *s, const char *delim);
uint64_t strtoull(const char *nptr, char **endptr, register int base);
size_t strxfrm(char *dest, const char *src, size_t n);
uint64_t atou64(const char *nptr);
uint8_t atou8(const char *nptr);
uint64_t strlen(const char *str);
void *memset(void *dest, int c, uint64_t n);
void strncpy(char *dest, const char *src, uint64_t n);
void *memcpy(void *dest, const void *src, uint64_t n);
uint64_t memcmp(const void *dest, const void *src, uint64_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint64_t n);
int zerocheck(const void *dest, uint64_t n);
//LE Movement functions
void store32(void* dest, uint32_t value);
void store16(void* dest, uint16_t value);
uint64_t load64(const void* src);
uint64_t load48(const void* src);
uint32_t load32(const void* src);
uint16_t load16(const void* src);
#endif