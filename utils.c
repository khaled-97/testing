/*
 * Utility Functions Implementation
 *
 * This module provides common utility functions used throughout the assembler:
 * 1. Memory management with error checking
 * 2. Error reporting with source line information
 * 3. String manipulation (ANSI C90 compliant)
 * 4. Label handling and validation
 * 5. Whitespace handling
 *
 * All string functions are implemented to be ANSI C90 compliant
 * and handle NULL pointers safely.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "utils.h"

/*
 * safe_malloc - Allocates memory with error checking
 *
 * Parameters:
 * size: Number of bytes to allocate
 *
 * Returns:
 * void*: Pointer to allocated memory
 *
 * Exits program if allocation fails to prevent undefined behavior
 */
void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Fatal: Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

/*
 * print_error - Prints formatted error message with source line info
 *
 * Parameters:
 * line: Source line information (file, line number)
 * format: printf-style format string
 * ...: Variable arguments for format string
 *
 * Prints error to stderr in format:
 * "Error in [filename] line [number]: [message]"
 */
void print_error(SourceLine line, const char *format, ...) {
    va_list args;
    fprintf(stderr, "Error in %s line %ld: ", line.filename, line.num);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

/*
 * skip_whitespace - Advances index past whitespace characters
 *
 * Parameters:
 * str: String to examine
 * index: Pointer to current position (updated)
 *
 * Skips spaces and tabs, updates index to first non-whitespace
 */
void skip_whitespace(const char *str, int *index) {
    while (str[*index] && (str[*index] == ' ' || str[*index] == '\t'))
        (*index)++;
}

/*
 * is_valid_label - Validates a potential label name
 *
 * Parameters:
 * name: String to check
 *
 * Returns:
 * Bool: TRUE if valid label, FALSE if not
 *
 * Rules:
 * 1. Must start with letter
 * 2. Can contain letters and numbers
 * 3. Length must be 1-31 characters
 */
Bool is_valid_label(const char *name) {
    int i;
    
    /* Must start with letter */
    if (!name || !isalpha(name[0]))
        return FALSE;
    
    /* Check remaining characters */
    for (i = 1; name[i]; i++) {
        if (!isalnum(name[i]))
            return FALSE;
        if (i >= MAX_SOURCE_LINE - 1)
            return FALSE;
    }
    
    /* Length check */
    if (i == 0 || i > 31)  /* Labels limited to 31 chars */
        return FALSE;
    
    return TRUE;
}

/*
 * get_label - Extracts label from beginning of source line
 *
 * Parameters:
 * line: Source line to examine
 * label_buf: Buffer to store extracted label
 *
 * Returns:
 * Bool: TRUE if label found, FALSE if not
 *
 * Extracts characters up to ':' if present
 */
Bool get_label(SourceLine line, char *label_buf) {
    int i = 0, j = 0;
    
    /* Skip initial whitespace */
    skip_whitespace(line.text, &i);
    
    /* Check for label */
    while (line.text[i] && line.text[i] != ':' && 
           line.text[i] != ' ' && line.text[i] != '\t' && 
           line.text[i] != '\n' && j < MAX_SOURCE_LINE - 1) {
        label_buf[j++] = line.text[i++];
    }
    label_buf[j] = '\0';
    
    /* If no colon, not a label */
    if (line.text[i] != ':') {
        label_buf[0] = '\0';
        return FALSE;
    }
    
    return TRUE;
}

/* 
 * String Manipulation Functions
 * All functions are ANSI C90 compliant alternatives to standard library
 */

/*
 * str_copy - Creates a new copy of a string
 *
 * Parameters:
 * src: Source string to copy
 *
 * Returns:
 * char*: Pointer to new copy, NULL if src is NULL
 *
 * Uses safe_malloc to allocate new memory
 */
char* str_copy(const char *src) {
    char *dst;
    size_t len;
    
    if (!src) return NULL;
    
    len = str_len(src) + 1;
    dst = (char*)safe_malloc(len);
    
    while ((*dst++ = *src++));
    return dst - len;
}

/*
 * str_trim - Removes leading and trailing whitespace
 *
 * Parameters:
 * str: String to trim (modified in place)
 *
 * Handles NULL strings safely
 */
void str_trim(char *str) {
    int i, j, len;
    
    if (!str) return;
    
    /* Trim leading whitespace */
    i = 0;
    while (str[i] && isspace(str[i])) i++;
    
    if (i > 0) {
        j = 0;
        while (str[i]) str[j++] = str[i++];
        str[j] = '\0';
    }
    
    /* Trim trailing whitespace */
    len = str_len(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

/*
 * ANSI C90 Compatible String Functions
 * Safe implementations that handle NULL pointers
 */

/*
 * str_len - Gets length of string
 *
 * Parameters:
 * str: String to measure
 *
 * Returns:
 * size_t: Length of string, 0 if NULL
 */
size_t str_len(const char *str) {
    size_t len = 0;
    if (str) while (str[len]) len++;
    return len;
}

/*
 * str_cmp - Compares two strings
 *
 * Parameters:
 * s1: First string
 * s2: Second string
 *
 * Returns:
 * int: <0 if s1<s2, 0 if equal, >0 if s1>s2
 * Handles NULL strings safely
 */
int str_cmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);
    
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/*
 * str_chr - Locates first occurrence of character in string
 *
 * Parameters:
 * str: String to search
 * c: Character to find
 *
 * Returns:
 * char*: Pointer to found character or NULL if not found
 */
char* str_chr(const char *str, int c) {
    if (!str) return NULL;
    
    while (*str && *str != c) str++;
    return (*str == c) ? (char*)str : NULL;
}
