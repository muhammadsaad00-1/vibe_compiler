# Makefile for VibeParser using Flex, Bison, and LLVM

# Tools
LEX = flex
YACC = bison
CC = clang
LLVM_CONFIG = /usr/local/opt/llvm@20/bin/llvm-config  # Explicit path for Homebrew LLVM 20

# Files
LEXER = vibeparser.l
PARSER = vibeparser.y
SRC = vibeparser_llvm.c
HEADERS = vibeparser_llvm.h
EXECUTABLE = vibecompiler
TEST_FILE = example.vibe
IR_FILE = output.ll

# Generated files
LEX_OUT = lex.yy.c
YACC_OUT_C = vibeparser.tab.c
YACC_OUT_H = vibeparser.tab.h

# LLVM configuration
LLVM_CFLAGS = $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS = $(shell $(LLVM_CONFIG) --libs core analysis executionengine bitwriter target)

# Compiler flags
CFLAGS = -g -Wall $(LLVM_CFLAGS) -I/usr/local/opt/llvm@20/include
LDFLAGS = $(LLVM_LDFLAGS) -L/usr/local/opt/llvm@20/lib
LIBS = $(LLVM_LIBS) -ll -ly  # -ll for macOS Flex library

# Targets
all: $(EXECUTABLE)

# Generate lexer
$(LEX_OUT): $(LEXER) $(YACC_OUT_H)
	$(LEX) -o $@ $<

# Generate parser
$(YACC_OUT_C) $(YACC_OUT_H): $(PARSER)
	$(YACC) -d -v -o $(YACC_OUT_C) $<

# Build compiler
$(EXECUTABLE): $(LEX_OUT) $(YACC_OUT_C) $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(LEX_OUT) $(YACC_OUT_C) $(SRC) $(LDFLAGS) $(LIBS)

# Generate LLVM IR
ir: $(EXECUTABLE)
	./$(EXECUTABLE) $(TEST_FILE) > $(IR_FILE)

# Run tests
test: $(EXECUTABLE)
	./$(EXECUTABLE) $(TEST_FILE)

# Debug parser conflicts
conflicts:
	$(YACC) -v -Wcounterexamples -o $(YACC_OUT_C) $(PARSER)

# Clean up
clean:
	rm -f $(LEX_OUT) $(YACC_OUT_C) $(YACC_OUT_H) $(EXECUTABLE) *.output *.ll *.o

# Phony targets
.PHONY: all ir test conflicts clean