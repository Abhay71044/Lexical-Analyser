CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -O2
TARGET = lexer

# Detect OS for binary extension and remove command
ifeq ($(OS),Windows_NT)
    TARGET_BIN = $(TARGET).exe
    RM = del /f /q
    RUN_PREFIX = .\
else
    TARGET_BIN = $(TARGET)
    RM = rm -f
    RUN_PREFIX = ./
endif

SRCS = main.c lexer.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET_BIN)
	$(RUN_PREFIX)$(TARGET_BIN) tests/test1_basic.c

test: $(TARGET_BIN)
	@echo ==================================================
	@echo Running Test 1: Basic C Program
	@echo ==================================================
	$(RUN_PREFIX)$(TARGET_BIN) tests/test1_basic.c
	@echo --------------------------------------------------
	@echo Running Test 2: C89/C99/C11 Keywords
	@echo --------------------------------------------------
	$(RUN_PREFIX)$(TARGET_BIN) tests/test2_keywords.c
	@echo --------------------------------------------------
	@echo Running Test 3: Numeric Constants
	@echo --------------------------------------------------
	$(RUN_PREFIX)$(TARGET_BIN) tests/test3_constants.c
	@echo --------------------------------------------------
	@echo Running Test 4: Operators and Maximal Munch
	@echo --------------------------------------------------
	$(RUN_PREFIX)$(TARGET_BIN) tests/test4_operators.c
	@echo --------------------------------------------------
	@echo Running Test 5: String/Char Literals and Escapes
	@echo --------------------------------------------------
	$(RUN_PREFIX)$(TARGET_BIN) tests/test5_strings_escapes.c
	@echo --------------------------------------------------
	@echo Running Test 6: Lexical Errors and Error Recovery
	@echo --------------------------------------------------
	-$(RUN_PREFIX)$(TARGET_BIN) tests/test6_errors.c
	@echo ==================================================
	@echo All test suites completed successfully!
	@echo ==================================================

clean:
	-$(RM) $(OBJS) $(TARGET_BIN)

valgrind: $(TARGET_BIN)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_PREFIX)$(TARGET_BIN) tests/test1_basic.c
	-valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(RUN_PREFIX)$(TARGET_BIN) tests/test6_errors.c

.PHONY: all run test clean valgrind
