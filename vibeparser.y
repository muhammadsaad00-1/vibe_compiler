%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mood.h"
#include "vibeparser_llvm.h"
Mood current_mood = NEUTRAL;
extern int yylex();
extern void init_llvm_ir_generation();
extern FILE *yyin;
void yyerror(const char *s);

/* Flag for controlling conditional execution */
int condition_result = 0;
int skip_until_endif = 0;  // New flag to skip statements when condition is false

/* Symbol table for storing variables */
struct symbol {
    char *name;
    int value;
    char *str_value;
    int is_string;
};

#define MAX_SYMBOLS 100
struct symbol symbol_table[MAX_SYMBOLS];
int sym_count = 0;

/* Function to add or update a symbol in the table */
int add_symbol(char *name, int value) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].value = value;
            symbol_table[i].is_string = 0;
            return i;
        }
    }
    
    if (sym_count >= MAX_SYMBOLS) {
        yyerror("Symbol table full");
        return -1;
    }
    
    symbol_table[sym_count].name = strdup(name);
    symbol_table[sym_count].value = value;
    symbol_table[sym_count].is_string = 0;
    return sym_count++;
}

/* Function to add or update a string symbol in the table */
int add_string_symbol(char *name, char *value) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            if (symbol_table[i].is_string && symbol_table[i].str_value) {
                free(symbol_table[i].str_value);
            }
            symbol_table[i].str_value = strdup(value);
            symbol_table[i].is_string = 1;
            return i;
        }
    }
    
    if (sym_count >= MAX_SYMBOLS) {
        yyerror("Symbol table full");
        return -1;
    }
    
    symbol_table[sym_count].name = strdup(name);
    symbol_table[sym_count].str_value = strdup(value);
    symbol_table[sym_count].is_string = 1;
    return sym_count++;
}

/* Function to get a symbol's value */
int get_symbol_value(char *name) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            if (symbol_table[i].is_string) {
                yyerror("Type error: Expected integer but got string");
                return 0;
            }
            return symbol_table[i].value;
        }
    }
    
    yyerror("Undefined variable");
    return 0;
}

/* Function to get a symbol's string value */
char* get_symbol_string(char *name) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            if (!symbol_table[i].is_string) {
                char buffer[32];
                sprintf(buffer, "%d", symbol_table[i].value);
                return strdup(buffer);
            }
            return strdup(symbol_table[i].str_value);
        }
    }
    
    yyerror("Undefined variable");
    return strdup("");
}

/* Function to check if symbol is a string */
int is_string_symbol(char *name) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].is_string;
        }
    }
    
    yyerror("Undefined variable");
    return 0;
}

/* Function to clean up the symbol table */
void cleanup_symbols() {
    for (int i = 0; i < sym_count; i++) {
        free(symbol_table[i].name);
        if (symbol_table[i].is_string && symbol_table[i].str_value) {
            free(symbol_table[i].str_value);
        }
    }
    sym_count = 0;
}
%}

%union {
    int num;
    char *str;
}

%token MOOD_SARCASTIC MOOD_ROMANTIC
%token LET BE AS RADIANT THE NUMBER IS
%token SARCASTIC_WOW SARCASTIC_NOW SARCASTIC_REV
%token PLUS MINUS TIMES DIVIDE MOD ASSIGN SEMICOLON
%token LPAREN RPAREN
%token PRINT IF THEN ELSE ENDIF
%token EQ GT LT NOT
%token <num> INTEGER
%token <str> IDENTIFIER STRING_LITERAL

/* Define operator precedence to resolve conflicts */
%left PLUS MINUS
%left TIMES DIVIDE MOD
%left EQ GT LT
%right NOT

/* Declare types for all non-terminals that return values */
%type <num> expression condition
%type <str> romantic_decl sarcastic_decl
%type <str> identifier_term print_value
%type <num> if_statement
%type <num> if_block if_true_block if_false_block

%%

program:
    mood_declaration statements
    ;

mood_declaration:
    MOOD_SARCASTIC { current_mood = SARCASTIC; }
    | MOOD_ROMANTIC { current_mood = ROMANTIC; }
    | /* empty */ { current_mood = NEUTRAL; }
    ;

statements:
    statements statement
    | statement
    | /* empty */
    ;

statement:
    variable_decl SEMICOLON
    | expression SEMICOLON { 
        if (current_mood != SARCASTIC && current_mood != ROMANTIC && !skip_until_endif) {
            printf("Expression result: %d\n", $1); 
        }
    }
    | print_statement SEMICOLON
    | if_statement
    ;

variable_decl:
    romantic_decl { if (!skip_until_endif) { printf("ROMANTIC DECL: %s\n", $1); free($1); } }
    | sarcastic_decl { if (!skip_until_endif) { printf("SARCASTIC DECL: %s\n", $1); free($1); } }
    | IDENTIFIER ASSIGN expression { if (!skip_until_endif) { add_symbol($1, $3); free($1); } }
    | IDENTIFIER ASSIGN STRING_LITERAL { if (!skip_until_endif) { add_string_symbol($1, $3); free($1); free($3); } }
    ;

romantic_decl:
    LET identifier_term BE AS RADIANT AS THE NUMBER INTEGER {
        $$ = malloc(100);
        sprintf($$, "int %s = %d;", $2, $9);
        if (!skip_until_endif) {
            add_symbol($2, $9);
        }
        free($2);
    }
    ;

sarcastic_decl:
    SARCASTIC_WOW identifier_term IS SARCASTIC_NOW INTEGER SARCASTIC_REV {
        $$ = malloc(100);
        sprintf($$, "int %s = %d; // Revolutionary", $2, $5);
        if (!skip_until_endif) {
            add_symbol($2, $5);
        }
        free($2);
    }
    ;

print_statement:
    PRINT print_value {
        if (!skip_until_endif) {
            if (current_mood == SARCASTIC) {
                printf("Oh wow, here's your output: %s", $2);
            } else if (current_mood == ROMANTIC) {
                printf("* softly whispers * %s", $2);
            } else {
                printf("Output: %s", $2);
            }
            printf("\n");
        }
        free($2);
    }
    ;

print_value:
    expression {
        char buffer[32];
        sprintf(buffer, "%d", $1);
        $$ = strdup(buffer);
    }
    | STRING_LITERAL { $$ = $1; }
    | identifier_term {
        if (is_string_symbol($1)) {
            $$ = get_symbol_string($1);
        } else {
            char buffer[32];
            sprintf(buffer, "%d", get_symbol_value($1));
            $$ = strdup(buffer);
        }
        free($1);
    }
    ;

if_statement:
    IF condition THEN {
        condition_result = $2;
        if (!condition_result) {
            skip_until_endif = 1;  // Skip until we find ELSE or ENDIF
        }
    } if_block {
        skip_until_endif = 0;  // Reset skipping flag
        $$ = $5;
    }
    ;

if_block:
    if_true_block ENDIF { $$ = condition_result; }
    | if_true_block ELSE {
        if (condition_result) {
            skip_until_endif = 1;  // Skip else block if condition was true
        } else {
            skip_until_endif = 0;  // Execute else block if condition was false
        }
    } if_false_block ENDIF {
        skip_until_endif = 0;  // Reset skipping flag
        $$ = condition_result;
    }
    ;

if_true_block:
    statements { $$ = condition_result; }
    ;

if_false_block:
    statements { $$ = condition_result; }
    ;

condition:
    expression EQ expression { $$ = ($1 == $3); }
    | expression GT expression { $$ = ($1 > $3); }
    | expression LT expression { $$ = ($1 < $3); }
    | NOT condition { $$ = !$2; }
    | LPAREN condition RPAREN { $$ = $2; }
    | expression { $$ = $1 != 0; } /* Non-zero is true */
    ;

identifier_term:
    IDENTIFIER { $$ = $1; }
    ;

expression:
    INTEGER { $$ = $1; }
    | identifier_term { 
        $$ = get_symbol_value($1);
        free($1);
    }
    | expression PLUS expression { $$ = $1 + $3; }
    | expression MINUS expression { $$ = $1 - $3; }
    | expression TIMES expression { $$ = $1 * $3; }
    | expression DIVIDE expression { 
        if ($3 == 0) {
            yyerror("Division by zero");
            $$ = 0;
        } else {
            $$ = $1 / $3; 
        }
    }
    | expression MOD expression {
        if ($3 == 0) {
            yyerror("Modulo by zero");
            $$ = 0;
        } else {
            $$ = $1 % $3;
        }
    }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    if (current_mood == SARCASTIC) {
        fprintf(stderr, "Oh great, you broke it: %s\n", s);
    } else if (current_mood == ROMANTIC) {
        fprintf(stderr, "Alas, an error has occurred like a fallen petal: %s\n", s);
    } else {
        fprintf(stderr, "Error: %s\n", s);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("Failed to open file");
            return 1;
        }
    }
    
    yyparse();
    
    if (argc > 1) {
        fclose(yyin);
    }
   
    cleanup_symbols();
     init_llvm_ir_generation();
    return 0;
}