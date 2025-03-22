# Compiler and flags
CC = gcc
CFLAGS = -ansi -pedantic -Wall
LDFLAGS = 

# Source files
SRCS = assembler.c \
       first_pass.c \
       second_pass.c \
       code.c \
       instructions.c \
       symbol_table.c \
       utils.c \
       writefiles.c \
       preprocessor.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = assembler

# Input file
INPUT = test1

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the assembler with the input file
run: $(TARGET)
	./$(TARGET) $(INPUT)

# Clean generated files
clean:
	rm -f $(OBJS) $(TARGET) $(INPUT).ob $(INPUT).ext $(INPUT).ent $(INPUT).am
