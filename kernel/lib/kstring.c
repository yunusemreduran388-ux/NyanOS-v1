/* We're in a freestanding environment (no libc), so here are our own
   minimal implementations of the string/memory functions we need. */
#include "kstring.h"

void* memset(void *dest, int val, size_t len) {
    uint8_t *d = (uint8_t*)dest;
    for (size_t i = 0; i < len; i++) d[i] = (uint8_t)val;
    return dest;
}

void* memcpy(void *dest, const void *src, size_t len) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    for (size_t i = 0; i < len; i++) d[i] = s[i];
    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

char* strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = 0;
    return dest;
}

char* strcat(char *dest, const char *src) {
    char *d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

void itoa(int value, char *buf, int base) {
    char tmp[32];
    int i = 0, neg = 0;
    unsigned int uv;

    if (value < 0 && base == 10) { neg = 1; uv = (unsigned int)(-value); }
    else uv = (unsigned int)value;

    if (uv == 0) tmp[i++] = '0';
    while (uv) {
        int d = uv % base;
        tmp[i++] = (d < 10) ? ('0' + d) : ('a' + d - 10);
        uv /= base;
    }
    if (neg) tmp[i++] = '-';

    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
