/* Utility functions implementation */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "utils.h"

/* Memory allocation with error checking */
void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Fatal: Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

/* Print error message with line info */
void print_error(SourceLine line, const char *format, ...) {
    va_list args;
    fprintf(stderr, "Error in %s line %ld: ", line.filename, line.num);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

/* Skip whitespace in a string */
void skip_whitespace(const char *str, int *index) {
    while (str[*index] && (str[*index] == ' ' || str[*index] == '\t'))
        (*index)++;
}

/* Check if a string is a valid label name */
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

/* Find and extract label from line if exists */
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

/* String manipulation */
char* str_copy(const char *src) {
    char *dst;
    size_t len;
    
    if (!src) return NULL;
    
    len = str_len(src) + 1;
    dst = (char*)safe_malloc(len);
    
    while ((*dst++ = *src++));
    return dst - len;
}

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

/* Safe string functions (ANSI C90 compatible) */
size_t str_len(const char *str) {
    size_t len = 0;
    if (str) while (str[len]) len++;
    return len;
}

int str_cmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);
    
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* str_chr(const char *str, int c) {
    if (!str) return NULL;
    
    while (*str && *str != c) str++;
    return (*str == c) ? (char*)str : NULL;
}
