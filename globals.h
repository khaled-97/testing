/* Global definitions for assembler project */
#ifndef GLOBALS_H
#define GLOBALS_H

/* Boolean type definition */
typedef enum { 
    FALSE = 0, 
    TRUE = 1 
} Bool;

/* System limits */
#define MAX_CODE_SIZE 1200    /* Maximum instructions memory */
#define MAX_SOURCE_LINE 81   /* Maximum input line length */
#define START_IC 100         /* Initial instruction counter */

/* Addressing modes */
typedef enum {
    IMMEDIATE = 0,   /* #value */
    DIRECT = 1,      /* label */
    RELATIVE = 2,    /* label[offset] */
    REGISTER_MODE = 3,/* r0-r7 */
    NO_ADDRESSING = -1,
    INVALID_ADDR = -2  /* Used for invalid addressing like r8 */
} AddressMode;

/* Operation codes */
typedef enum {
    /* Group A */
    OP_MOV = 0,
    OP_CMP = 1,
    OP_MATH = 2,     /* Shared by ADD/SUB */
    
    OP_LEA = 4,
    
    /* Group B */
    OP_SINGLE = 5,   /* CLR/NOT/INC/DEC */
    
    OP_JUMPS = 9,    /* JMP/BNE/JSR */
    
    OP_RED = 12,
    OP_PRN = 13,
    
    /* Group C */
    OP_RTS = 14,
    OP_HALT = 15,
    
    OP_INVALID = -1
} OpCode;

/* Function identifiers */
typedef enum {
    F_ADD = 1,
    F_SUB = 2,
    
    F_CLR = 1,
    F_NOT = 2,
    F_INC = 3,
    F_DEC = 4,
    
    F_JMP = 1,
    F_BNE = 2,
    F_JSR = 3,
    
    F_NONE = 0
} FuncCode;

/* CPU registers */
typedef enum {
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    NO_REGISTER = -1
} RegNum;

/* Instruction word structure */
typedef struct {
    unsigned are : 3;
    unsigned func : 5;
    unsigned dest_reg : 3;
    unsigned dest_mode : 2;
    unsigned src_reg : 3;
    unsigned src_mode : 2;
    unsigned op : 6;
} InstructionWord;

/* Data word structure */
typedef struct {
    unsigned are : 3;
    unsigned long value;
} DataWord;

/* Unified machine word */
typedef struct {
    short is_instruction;  /* Non-zero for instructions */
    union {
        DataWord* data;
        InstructionWord* code;
    } content;
} MachineWord;

/* Directive types */
typedef enum {
    DIR_DATA,
    DIR_EXTERN,
    DIR_ENTRY,
    DIR_STRING,
    DIR_NONE,
    DIR_ERROR
} Directive;

/* Source line metadata */
typedef struct {
    long num;        /* Line number */
    const char* filename;  /* Source file */
    char* text;      /* Line content */
} SourceLine;

#endif /* GLOBALS_H */
