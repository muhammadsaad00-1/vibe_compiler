CC = gcc
CFLAGS = -Wall -Wextra -g

# Detect OS and set appropriate flex library
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS uses -ll instead of -lfl
    FLEX_LIB = -ll
else
    # Linux and others use -lfl
    FLEX_LIB = -lfl
endif

all: vibeparser

vibeparser: vibeparser.tab.c lex.yy.c
	$(CC) $(CFLAGS) -o vibeparser vibeparser.tab.c lex.yy.c $(FLEX_LIB)

vibeparser.tab.c vibeparser.tab.h: vibeparser.y
	bison -d vibeparser.y

lex.yy.c: vibeparser.l vibeparser.tab.h
	flex vibeparser.l

clean:
	rm -f vibeparser lex.yy.c vibeparser.tab.c vibeparser.tab.h

run: vibeparser
	./vibeparser example.vibe

.PHONY: all clean run