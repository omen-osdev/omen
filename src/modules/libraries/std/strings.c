#include <omen/libraries/std/string.h>
#include <omen/libraries/std/ctype.h>

void *memchr(const void *s, int c, size_t n) {__UNDEFINED();}
void *memmove(void *dest, const void *src, size_t n) {__UNDEFINED();}
void *strcat(char *dest, const char *src) {
    char *dest_start = dest;
    while (*dest != '\0') {
        dest++;
    }
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return dest_start;
}
void *strncat(char *dest, const char *src, size_t n) {
    char *dest_start = dest;
    while (*dest != '\0') {
        dest++;
    }
    while (*src != '\0' && n > 0) {
        *dest = *src;
        dest++;
        src++;
        n--;
    }
    *dest = '\0';
    return dest_start;
}
void *strchr(const char *str, int ch) {
    while (*str != '\0') {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }
    if (*str == ch) {
        return (char *)str;
    }
    return 0;
}
int strcoll(const char *s1, const char *s2) {__UNDEFINED();}
size_t strcspn(const char *str, const char *reject) {
    const char *s = str;
    size_t count = 0;

    while (*s) {
        const char *r = reject;
        while (*r) {
            if (*s == *r) {
                return count;
            }
            r++;
        }
        s++;
        count++;
    }

    return count;
}
char *strerror(int errnum) {__UNDEFINED();}
char *strpbrk(const char *s, const char *accept) {__UNDEFINED();}
char *strrchr(const char *str, int ch) {
    char *last = 0;
    while (*str != '\0') {
        if (*str == ch) {
            last = (char *)str;
        }
        str++;
    }
    if (*str == ch) {
        last = (char *)str;
    }
    return last;
}
size_t strspn(const char *s, const char *accept) {__UNDEFINED();}
char *strstr(const char *haystack, const char *needle) {
    while (*haystack != '\0') {
        const char *h = haystack;
        const char *n = needle;
        while (*h == *n && *n != '\0') {
            h++;
            n++;
        }
        if (*n == '\0') {
            return (char *)haystack;
        }
        haystack++;
    }
    return 0;
}
size_t strxfrm(char *dest, const char *src, size_t n) {__UNDEFINED();}

uint8_t atou8(const char *nptr) {
    return (uint8_t)atou64(nptr);
}

uint64_t atou64(const char *nptr) {
    
    //Check if prefix is 0x or 0b for hex or binary
    while (isspace(*nptr)) {
        nptr++;
    }
    uint64_t base = 10;
    if (nptr[0] == '0' && nptr[1] == 'x') {
        base = 16;
        nptr += 2;
    } else if (nptr[0] == '0' && nptr[1] == 'b') {
        base = 2;
        nptr += 2;
    }

    while (!isdigit(*nptr)) {
        nptr++;
    }

    uint64_t result = 0;
    uint64_t multiplier = 1;
    uint64_t len = strlen(nptr);
    for (uint64_t i = 0; i < len; i++) {
        char c = nptr[len - i - 1];
        uint64_t digit = 0;

        if (isdigit(c)) {
            digit = c - '0'; // Convert decimal digit
        } else if (isxdigit(c)) {
            if (c >= 'a' && c <= 'f') {
                digit = c - 'a' + 10; // Convert lowercase hex digit
            } else if (c >= 'A' && c <= 'F') {
                digit = c - 'A' + 10; // Convert uppercase hex digit
            }
        }

        result += digit * multiplier;
        multiplier *= base;
    }
    return result;
}

uint64_t strtoull(const char *nptr, char **endptr, register int base)
{
	register const char *s = nptr;
	register uint64_t acc;
	register int c;
	register uint64_t cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (uint64_t)ULLONG_MAX / (uint64_t)base;
	cutlim = (uint64_t)ULLONG_MAX % (uint64_t)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULLONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

char* strtok(char* s, const char* delim) {
    static char* lastToken = 0;
    if (s != 0) {
        lastToken = s;
    } else if (lastToken == 0) {
        return 0;
    }

    char* tokenStart = lastToken;

    while (*lastToken != '\0') {
        const char* d = delim;
        while (*d != '\0') {
            if (*lastToken == *d) {
                *lastToken = '\0';
                lastToken++;
                if (tokenStart == lastToken - 1) {
                    tokenStart = lastToken;
                    continue;
                }
                return tokenStart;
            }
            d++;
        }
        lastToken++;
    }

    if (tokenStart == lastToken) {
        return 0;
    }

    return tokenStart;
}

uint64_t strlen(const char *str) {
    uint64_t len = 0;
    while (*str++ && len < STR_MAX_SIZE) {
        len++;
    }
    return len;
}

void *memset(void *dest, int val, uint64_t size) {
    uint8_t *d = (uint8_t *)dest;
    for (uint64_t i = 0; i < size; i++) {
        d[i] = val;
    }
    return dest;
}

char *strcpy(char *dest, const char *src) {
    uint64_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

void strncpy(char *dest, const char *src, uint64_t n) {
    uint64_t i = 0;
    if (strlen(src) < n) {
        n = strlen(src);
    }
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void *memcpy(void *dest, const void *src, uint64_t size) {
    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;
    for (uint64_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dest;
}

int zerocheck(const void *dest, uint64_t size) {
    uint8_t *d = (uint8_t *)dest;
    for (uint64_t i = 0; i < size; i++) {
        if (d[i] != 0) {
            return i;
        }
    }
    return -1;
}

 uint64_t memcmp(const void *dest, const void *src, uint64_t size) {
    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;
    for (uint64_t i = 0; i < size; i++) {
        if (d[i] != s[i]) {
            return 1;
        }
    }
    return 0;
 }

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, uint64_t n) {
	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(const unsigned char *)s1 - *(const unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}

 
void store32(void* dest, uint32_t value) {
	uint8_t* dest_ptr = (uint8_t *)dest;
	*dest_ptr++ = (uint8_t)value;
	value >>= 8;
	*dest_ptr++ = (uint8_t)value;
	value >>= 8;
	*dest_ptr++ = (uint8_t)value;
	value >>= 8;
	*dest_ptr++ = (uint8_t)value;
}

void store16(void* dest, uint16_t value) {
	uint8_t* dest_ptr = (uint8_t *)dest;
	*dest_ptr++ = (uint8_t)value;
	value >>= 8;
	*dest_ptr++ = (uint8_t)value;
}

uint64_t load64(const void* src) {
    uint64_t value = 0;
    const uint8_t* src_ptr = (const uint8_t *)src;
    value |= *src_ptr++;
    value |= (*src_ptr++ << 8);
    value |= (*src_ptr++ << 16);
    value |= (*src_ptr++ << 24);
    value |= ((uint64_t)*src_ptr++ << 32);
    value |= ((uint64_t)*src_ptr++ << 40);
    value |= ((uint64_t)*src_ptr++ << 48);
    value |= ((uint64_t)*src_ptr++ << 56);
    return value;
}

uint64_t load48(const void* src) {
    uint64_t value = 0;
    const uint8_t* src_ptr = (const uint8_t *)src;
    value |= *src_ptr++;
    value |= (*src_ptr++ << 8);
    value |= (*src_ptr++ << 16);
    value |= (*src_ptr++ << 24);
    value |= ((uint64_t)*src_ptr++ << 32);
    value |= ((uint64_t)*src_ptr++ << 40);
    return value;
}

uint32_t load32(const void* src) {
	uint32_t value = 0;
	const uint8_t* src_ptr = (const uint8_t *)src;
	value |= *src_ptr++;
	value |= (*src_ptr++ << 8);
	value |= (*src_ptr++ << 16);
	value |= (*src_ptr++ << 24);
	return value;
}

uint16_t load16(const void* src) {
	uint16_t value = 0;
	const uint8_t* src_ptr = (const uint8_t *)src;
	value |= *src_ptr++;
	value |= (*src_ptr++ << 8);
	return value;
}