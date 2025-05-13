%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mood.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include "vibeparser_llvm.h"

Mood current_mood = NEUTRAL;
extern int yylex();
extern FILE *yyin;
extern ForContext* get_current_for_context();
extern void end_for_loop(char *var_name);
void yyerror(const char *s);

/* Flag for controlling conditional execution */
int condition_result = 0;
int skip_until_endif = 0;

/* Symbol table for storing variables */
struct symbol {
    char *name;
    int value;
    char *str_value;
    int is_string;
    LLVMValueRef llvm_value;  // LLVM value reference
};

#define MAX_SYMBOLS 100
struct symbol symbol_table[MAX_SYMBOLS];
int sym_count = 0;

/* LLVM global variables - declared extern to use from vibeparser_llvm.c */
extern LLVMModuleRef module;
extern LLVMBuilderRef builder;
extern LLVMExecutionEngineRef engine;
extern LLVMPassManagerRef pass_manager;
extern LLVMValueRef current_function;
extern LLVMBasicBlockRef current_block;

/* Function to add or update a symbol in the table */
int add_symbol(char *name, int value) {
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].value = value;
            symbol_table[i].is_string = 0;
            
            // Generate LLVM IR for variable assignment
            if (!skip_until_endif) {
                LLVMValueRef const_val = LLVMConstInt(LLVMInt32Type(), value, 0);
                gen_variable_decl(name, const_val, 0);
                symbol_table[i].llvm_value = get_symbol_value_llvm(name);
            }
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
    
    // Generate LLVM IR for new variable
    if (!skip_until_endif) {
        LLVMValueRef const_val = LLVMConstInt(LLVMInt32Type(), value, 0);
        gen_variable_decl(name, const_val, 0);
        symbol_table[sym_count].llvm_value = get_symbol_value_llvm(name);
    }
    
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
            
            // Generate LLVM IR for string variable
            if (!skip_until_endif) {
                LLVMValueRef str_val = LLVMBuildGlobalStringPtr(builder, value, "str_lit");
                gen_variable_decl(name, str_val, 1);
                symbol_table[i].llvm_value = get_symbol_value_llvm(name);
            }
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
    
    // Generate LLVM IR for new string variable
    if (!skip_until_endif) {
        LLVMValueRef str_val = LLVMBuildGlobalStringPtr(builder, value, "str_lit");
        gen_variable_decl(name, str_val, 1);
        symbol_table[sym_count].llvm_value = get_symbol_value_llvm(name);
    }
    
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

/* Loop implementation - we'll store the statements to execute */
typedef struct {
    char *loop_var;
    int start_value;
    int end_value;
    int current_value;
    /* We can't easily store statements in Bison, so we'll use a different approach */
} LoopContext;

static LoopContext current_loop;
static int in_loop = 0;
%}
%code requires {
    #include "vibeparser_llvm.h"
}

%union {
    int num;
    char *str;
    ExprResult expr_result;
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
%token SARCASTIC_FOR SARCASTIC_WITH SARCASTIC_UNTIL SARCASTIC_NEXT
%token ROMANTIC_FOR ROMANTIC_WITH ROMANTIC_UNTIL ROMANTIC_NEXT

/* Define operator precedence to resolve conflicts */
%left PLUS MINUS
%left TIMES DIVIDE MOD
%left EQ GT LT
%right NOT

/* Declare types for all non-terminals that return values */
%type <expr_result> expression condition
%type <str> romantic_decl sarcastic_decl
%type <str> identifier_term print_value
%type <num> if_statement
%type <num> if_block if_true_block if_false_block
%type <num> for_statement
%%

program:
    mood_declaration statements {
        if (!skip_until_endif) {
            finalize_llvm("output.ll");
        }
    }
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
            printf("Expression result: %d\n", $1.value); 
        }
    }
    | print_statement SEMICOLON
    | if_statement
    | for_statement
    ;

variable_decl:
    romantic_decl { if (!skip_until_endif) { printf("ROMANTIC DECL: %s\n", $1); free($1); } }
    | sarcastic_decl { if (!skip_until_endif) { printf("SARCASTIC DECL: %s\n", $1); free($1); } }
    | IDENTIFIER ASSIGN expression { 
        if (!skip_until_endif) { 
            add_symbol($1, $3.value); 
            free($1); 
        } 
    }
    | IDENTIFIER ASSIGN STRING_LITERAL { 
        if (!skip_until_endif) { 
            add_string_symbol($1, $3); 
            free($1); 
            free($3); 
        } 
    }
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
            
            // Generate LLVM IR for print statement
            LLVMValueRef print_val = NULL;
            int is_string = 0;
            
            // Check if it's a string literal or variable
            for (int i = 0; i < sym_count; i++) {
                if (symbol_table[i].name && strcmp(symbol_table[i].name, $2) == 0) {
                    is_string = symbol_table[i].is_string;
                    if (is_string) {
                        print_val = LLVMBuildLoad2(builder, LLVMPointerType(LLVMInt8Type(), 0), 
                                                   symbol_table[i].llvm_value, "strload");
                    } else {
                        print_val = LLVMBuildLoad2(builder, LLVMInt32Type(), 
                                                   symbol_table[i].llvm_value, "intload");
                    }
                    break;
                }
            }
            
            // If it's not a variable, it might be a literal
            if (print_val == NULL) {
                // Try to parse as integer
                char *endptr;
                long val = strtol($2, &endptr, 10);
                if (*endptr == '\0') {
                    // It's an integer literal
                    print_val = LLVMConstInt(LLVMInt32Type(), val, 0);
                    is_string = 0;
                } else {
                    // It's a string literal
                    print_val = LLVMBuildGlobalStringPtr(builder, $2, "str_print");
                    is_string = 1;
                }
            }
            
            gen_print(print_val, is_string);
        }
        free($2);
    }
    ;

print_value:
    expression {
        char buffer[32];
        sprintf(buffer, "%d", $1.value);
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
        condition_result = $2.value;
        if (!condition_result) {
            skip_until_endif = 1;
        }
        
        // Generate LLVM IR for if statement
        if (!skip_until_endif && $2.llvm_value) {
            start_if_statement($2.llvm_value);
        }
    } if_block {
        skip_until_endif = 0;
        $$ = $5;
    }
    ;

if_block:
    if_true_block ENDIF { 
        $$ = condition_result; 
        // End if statement in LLVM IR
        if (!skip_until_endif) {
            end_if_statement();
        }
    }
    | if_true_block ELSE {
        if (condition_result) {
            skip_until_endif = 1;
        } else {
            skip_until_endif = 0;
        }
        
        // Generate LLVM IR for else part
        if (!skip_until_endif) {
            handle_else();
        }
    } if_false_block ENDIF {
        skip_until_endif = 0;
        $$ = condition_result;
        
        // End if statement in LLVM IR
        if (!skip_until_endif) {
            end_if_statement();
        }
    }
    ;

if_true_block:
    statements { $$ = condition_result; }
    ;

if_false_block:
    statements { $$ = condition_result; }
    ;

condition:
    expression EQ expression { 
        int result = ($1.value == $3.value);
        $$.value = result;
        if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
            $$.llvm_value = gen_binary_op($1.llvm_value, EQ, $3.llvm_value);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | expression GT expression { 
        int result = ($1.value > $3.value);
        $$.value = result;
        if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
            $$.llvm_value = gen_binary_op($1.llvm_value, GT, $3.llvm_value);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | expression LT expression { 
        int result = ($1.value < $3.value);
        $$.value = result;
        if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
            $$.llvm_value = gen_binary_op($1.llvm_value, LT, $3.llvm_value);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | NOT condition { 
        $$.value = !$2.value;
        if (!skip_until_endif && $2.llvm_value) {
            $$.llvm_value = LLVMBuildNot(builder, $2.llvm_value, "nottmp");
        } else {
            $$.llvm_value = NULL;
        }
    }
    | LPAREN condition RPAREN { $$ = $2; }
    | expression { 
        $$.value = $1.value != 0;
        if (!skip_until_endif && $1.llvm_value) {
            LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);
            $$.llvm_value = LLVMBuildICmp(builder, LLVMIntNE, $1.llvm_value, zero, "condtmp");
        } else {
            $$.llvm_value = NULL;
        }
    }
    ;

identifier_term:
    IDENTIFIER { $$ = $1; }
    ;

expression:
    INTEGER { 
        $$.value = $1;
        if (!skip_until_endif) {
            $$.llvm_value = gen_expression($1);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | identifier_term { 
        $$.value = get_symbol_value($1);
        if (!skip_until_endif) {
            LLVMValueRef sym_val = get_symbol_value_llvm($1);
            if (sym_val) {
                $$.llvm_value = LLVMBuildLoad2(builder, LLVMInt32Type(), sym_val, "varload");
            } else {
                $$.llvm_value = NULL;
            }
        } else {
            $$.llvm_value = NULL;
        }
        free($1);
    }
    | expression PLUS expression { 
        $$.value = $1.value + $3.value;
        if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
            $$.llvm_value = gen_binary_op($1.llvm_value, PLUS, $3.llvm_value);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | expression MINUS expression { 
        $$.value = $1.value - $3.value;
        if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
            $$.llvm_value = gen_binary_op($1.llvm_value, MINUS, $3.llvm_value);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | expression TIMES expression { 
        $$.value = $1.value * $3.value;
        if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
            $$.llvm_value = gen_binary_op($1.llvm_value, TIMES, $3.llvm_value);
        } else {
            $$.llvm_value = NULL;
        }
    }
    | expression DIVIDE expression { 
        if ($3.value == 0) {
            yyerror("Division by zero");
            $$.value = 0;
            $$.llvm_value = NULL;
        } else {
            $$.value = $1.value / $3.value;
            if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
                $$.llvm_value = gen_binary_op($1.llvm_value, DIVIDE, $3.llvm_value);
            } else {
                $$.llvm_value = NULL;
            }
        }
    }
    | expression MOD expression {
        if ($3.value == 0) {
            yyerror("Modulo by zero");
            $$.value = 0;
            $$.llvm_value = NULL;
        } else {
            $$.value = $1.value % $3.value;
            if (!skip_until_endif && $1.llvm_value && $3.llvm_value) {
                $$.llvm_value = gen_binary_op($1.llvm_value, MOD, $3.llvm_value);
            } else {
                $$.llvm_value = NULL;
            }
        }
    }
    | LPAREN expression RPAREN { $$ = $2; }
    ;

for_statement:
    SARCASTIC_FOR IDENTIFIER SARCASTIC_WITH expression SARCASTIC_UNTIL expression {
        if (!skip_until_endif) {
            // Initialize loop
            current_loop.loop_var = strdup($2);
            current_loop.start_value = $4.value;
            current_loop.end_value = $6.value;
            current_loop.current_value = current_loop.start_value;
            in_loop = 1;
            
            // Add loop variable to symbol table
            add_symbol($2, current_loop.current_value);
            
            // Mood-specific output
            if (current_mood == SARCASTIC) {
                printf("Oh great, another loop from %d to %d. How thrilling.\n", 
                       current_loop.start_value, current_loop.end_value);
            }
            
            // Execute loop manually
            for (int i = current_loop.start_value; i <= current_loop.end_value; i++) {
                current_loop.current_value = i;
                add_symbol(current_loop.loop_var, i);
                
                // Since we can't easily re-execute statements in Bison, 
                // we'll handle this in a simplified way for now
                printf("Loop iteration: %s = %d\n", current_loop.loop_var, i);
            }
            
            free(current_loop.loop_var);
            in_loop = 0;
        }
        free($2);
    } statements SARCASTIC_NEXT {
        if (!skip_until_endif && current_mood == SARCASTIC) {
            printf("Wow, that loop is finally over. Shocking.\n");
        }
        $$ = 1;
    }
    | ROMANTIC_FOR IDENTIFIER ROMANTIC_WITH expression ROMANTIC_UNTIL expression {
        if (!skip_until_endif) {
            // Initialize loop
            current_loop.loop_var = strdup($2);
            current_loop.start_value = $4.value;
            current_loop.end_value = $6.value;
            current_loop.current_value = current_loop.start_value;
            in_loop = 1;
            
            // Add loop variable to symbol table
            add_symbol($2, current_loop.current_value);
            
            if (current_mood == ROMANTIC) {
                printf("Let us dance together, %s, from %d until %d, like lovers under the moonlight.\n", 
                       $2, current_loop.start_value, current_loop.end_value);
            }
            
            // Execute loop manually
            for (int i = current_loop.start_value; i <= current_loop.end_value; i++) {
                current_loop.current_value = i;
                add_symbol(current_loop.loop_var, i);
                
                // Since we can't easily re-execute statements in Bison, 
                // we'll handle this in a simplified way for now
                printf("Loop iteration: %s = %d\n", current_loop.loop_var, i);
            }
            
            free(current_loop.loop_var);
            in_loop = 0;
        }
        free($2);
    } statements ROMANTIC_NEXT {
        if (!skip_until_endif && current_mood == ROMANTIC) {
            printf("Our dance comes to an end, like a gentle sunset.\n");
        }
        $$ = 1;
    }
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
    
    // Initialize LLVM before parsing
    init_llvm();
    
    yyparse();
    
    if (argc > 1) {
        fclose(yyin);
    }
   
    cleanup_symbols();
    cleanup_llvm();
    return 0;
}