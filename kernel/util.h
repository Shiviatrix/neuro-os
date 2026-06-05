#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

void memory_copy(uint8_t *source, uint8_t *dest, int nbytes);
void memory_set(uint8_t *dest, uint8_t val, uint32_t len);
int string_length(char s[]);
void reverse(char s[]);
void int_to_ascii(int n, char str[]);
void append(char s[], char n);
void backspace(char s[]);
int strcmp(char s1[], char s2[]);
int strncmp(const char s1[], const char s2[], int n);

#endif
