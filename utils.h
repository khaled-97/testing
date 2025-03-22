/* Utility functions for the assembler */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "globals.h"

/* Memory allocation with error checking */
void* safe_malloc(size_t size);

/* Print error message with line info */
void print_error(SourceLine line, const char *format, ...);

/* Skip whitespace in a string */
void skip_whitespace(const char *str, int *index);

/* Check if a string is a valid label name */
Bool is_valid_label(const char *name);

/* Find and extract label from line if exists */
Bool get_label(SourceLine line, char *label_buf);

/* String manipulation */
char* str_copy(const char *src);
void str_trim(char *str);

/* Safe string functions (ANSI C90 compatible) */
size_t str_len(const char *str);
int str_cmp(const char *s1, const char *s2);
char* str_chr(const char *str, int c);

#endif /* UTILS_H */
