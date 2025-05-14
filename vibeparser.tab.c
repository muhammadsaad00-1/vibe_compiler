/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "vibeparser.y"

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

#line 259 "vibeparser.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "vibeparser.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_MOOD_SARCASTIC = 3,             /* MOOD_SARCASTIC  */
  YYSYMBOL_MOOD_ROMANTIC = 4,              /* MOOD_ROMANTIC  */
  YYSYMBOL_LET = 5,                        /* LET  */
  YYSYMBOL_BE = 6,                         /* BE  */
  YYSYMBOL_AS = 7,                         /* AS  */
  YYSYMBOL_RADIANT = 8,                    /* RADIANT  */
  YYSYMBOL_THE = 9,                        /* THE  */
  YYSYMBOL_NUMBER = 10,                    /* NUMBER  */
  YYSYMBOL_IS = 11,                        /* IS  */
  YYSYMBOL_SARCASTIC_WOW = 12,             /* SARCASTIC_WOW  */
  YYSYMBOL_SARCASTIC_NOW = 13,             /* SARCASTIC_NOW  */
  YYSYMBOL_SARCASTIC_REV = 14,             /* SARCASTIC_REV  */
  YYSYMBOL_PLUS = 15,                      /* PLUS  */
  YYSYMBOL_MINUS = 16,                     /* MINUS  */
  YYSYMBOL_TIMES = 17,                     /* TIMES  */
  YYSYMBOL_DIVIDE = 18,                    /* DIVIDE  */
  YYSYMBOL_MOD = 19,                       /* MOD  */
  YYSYMBOL_ASSIGN = 20,                    /* ASSIGN  */
  YYSYMBOL_SEMICOLON = 21,                 /* SEMICOLON  */
  YYSYMBOL_LPAREN = 22,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 23,                    /* RPAREN  */
  YYSYMBOL_PRINT = 24,                     /* PRINT  */
  YYSYMBOL_IF = 25,                        /* IF  */
  YYSYMBOL_THEN = 26,                      /* THEN  */
  YYSYMBOL_ELSE = 27,                      /* ELSE  */
  YYSYMBOL_ENDIF = 28,                     /* ENDIF  */
  YYSYMBOL_EQ = 29,                        /* EQ  */
  YYSYMBOL_GT = 30,                        /* GT  */
  YYSYMBOL_LT = 31,                        /* LT  */
  YYSYMBOL_NOT = 32,                       /* NOT  */
  YYSYMBOL_INTEGER = 33,                   /* INTEGER  */
  YYSYMBOL_IDENTIFIER = 34,                /* IDENTIFIER  */
  YYSYMBOL_STRING_LITERAL = 35,            /* STRING_LITERAL  */
  YYSYMBOL_SARCASTIC_FOR = 36,             /* SARCASTIC_FOR  */
  YYSYMBOL_SARCASTIC_WITH = 37,            /* SARCASTIC_WITH  */
  YYSYMBOL_SARCASTIC_UNTIL = 38,           /* SARCASTIC_UNTIL  */
  YYSYMBOL_SARCASTIC_NEXT = 39,            /* SARCASTIC_NEXT  */
  YYSYMBOL_ROMANTIC_FOR = 40,              /* ROMANTIC_FOR  */
  YYSYMBOL_ROMANTIC_WITH = 41,             /* ROMANTIC_WITH  */
  YYSYMBOL_ROMANTIC_UNTIL = 42,            /* ROMANTIC_UNTIL  */
  YYSYMBOL_ROMANTIC_NEXT = 43,             /* ROMANTIC_NEXT  */
  YYSYMBOL_YYACCEPT = 44,                  /* $accept  */
  YYSYMBOL_program = 45,                   /* program  */
  YYSYMBOL_mood_declaration = 46,          /* mood_declaration  */
  YYSYMBOL_statements = 47,                /* statements  */
  YYSYMBOL_statement = 48,                 /* statement  */
  YYSYMBOL_variable_decl = 49,             /* variable_decl  */
  YYSYMBOL_romantic_decl = 50,             /* romantic_decl  */
  YYSYMBOL_sarcastic_decl = 51,            /* sarcastic_decl  */
  YYSYMBOL_print_statement = 52,           /* print_statement  */
  YYSYMBOL_print_value = 53,               /* print_value  */
  YYSYMBOL_if_statement = 54,              /* if_statement  */
  YYSYMBOL_55_1 = 55,                      /* $@1  */
  YYSYMBOL_if_block = 56,                  /* if_block  */
  YYSYMBOL_57_2 = 57,                      /* $@2  */
  YYSYMBOL_if_true_block = 58,             /* if_true_block  */
  YYSYMBOL_if_false_block = 59,            /* if_false_block  */
  YYSYMBOL_condition = 60,                 /* condition  */
  YYSYMBOL_identifier_term = 61,           /* identifier_term  */
  YYSYMBOL_expression = 62,                /* expression  */
  YYSYMBOL_for_statement = 63,             /* for_statement  */
  YYSYMBOL_64_3 = 64,                      /* $@3  */
  YYSYMBOL_65_4 = 65                       /* $@4  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   152

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  22
/* YYNRULES -- Number of rules.  */
#define YYNRULES  49
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  103

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   298


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   226,   226,   234,   235,   236,   240,   241,   242,   246,
     247,   252,   253,   254,   258,   259,   260,   266,   276,   287,
     298,   351,   356,   357,   370,   370,   387,   394,   394,   417,
     421,   425,   434,   443,   452,   460,   461,   473,   477,   485,
     499,   507,   515,   523,   537,   551,   555,   555,   593,   593
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "MOOD_SARCASTIC",
  "MOOD_ROMANTIC", "LET", "BE", "AS", "RADIANT", "THE", "NUMBER", "IS",
  "SARCASTIC_WOW", "SARCASTIC_NOW", "SARCASTIC_REV", "PLUS", "MINUS",
  "TIMES", "DIVIDE", "MOD", "ASSIGN", "SEMICOLON", "LPAREN", "RPAREN",
  "PRINT", "IF", "THEN", "ELSE", "ENDIF", "EQ", "GT", "LT", "NOT",
  "INTEGER", "IDENTIFIER", "STRING_LITERAL", "SARCASTIC_FOR",
  "SARCASTIC_WITH", "SARCASTIC_UNTIL", "SARCASTIC_NEXT", "ROMANTIC_FOR",
  "ROMANTIC_WITH", "ROMANTIC_UNTIL", "ROMANTIC_NEXT", "$accept", "program",
  "mood_declaration", "statements", "statement", "variable_decl",
  "romantic_decl", "sarcastic_decl", "print_statement", "print_value",
  "if_statement", "$@1", "if_block", "$@2", "if_true_block",
  "if_false_block", "condition", "identifier_term", "expression",
  "for_statement", "$@3", "$@4", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-51)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-24)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      11,   -51,   -51,    29,    85,   -51,    -9,    -9,     0,    36,
     102,   -51,     3,    10,    12,    85,   -51,    44,   -51,   -51,
      47,   -51,   -51,   131,   -51,   -51,    71,    67,     1,   -51,
     -51,    58,   122,   102,   102,    73,   114,    59,    48,    63,
     -51,   -51,   -51,     0,     0,     0,     0,     0,   -51,    93,
      90,   -51,    83,    97,   -51,   -51,     0,     0,     0,   -51,
     122,     0,     0,    43,    43,   -51,   -51,   -51,   100,    72,
     -51,    85,   122,   122,   122,    -6,   -11,   104,   103,    85,
     -51,    25,     0,     0,   113,   -51,   -51,   -51,   122,   122,
     132,    85,    85,    85,   118,    85,    95,    62,    23,   -51,
     -51,   -51,   -51
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       5,     3,     4,     0,     8,     1,     0,     0,     0,     0,
       0,    38,    37,     0,     0,     2,     7,     0,    14,    15,
       0,    12,    39,     0,    13,    37,     0,     0,     0,    22,
      20,    39,    21,     0,     0,     0,    36,     0,     0,     0,
       6,     9,    11,     0,     0,     0,     0,     0,    10,     0,
       0,    45,     0,     0,    34,    24,     0,     0,     0,    17,
      16,     0,     0,    40,    41,    42,    43,    44,     0,     0,
      35,     8,    31,    32,    33,     0,     0,     0,     0,    29,
      25,     0,     0,     0,     0,    19,    27,    26,    46,    48,
       0,     8,     8,     8,     0,    30,     0,     0,     0,    18,
      28,    47,    49
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -51,   -51,   -51,   -50,   -15,   -51,   -51,   -51,   -51,   -51,
     -51,   -51,   -51,   -51,   -51,   -51,    39,    82,    -7,   -51,
     -51,   -51
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     3,     4,    15,    16,    17,    18,    19,    20,    30,
      21,    71,    80,    91,    81,    96,    35,    22,    23,    24,
      92,    93
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      40,    28,    32,    36,    43,    44,    45,    46,    47,    43,
      44,    45,    46,    47,     1,     2,    43,    44,    45,    46,
      47,    79,     8,    37,    51,    25,    53,    36,     6,     5,
      60,    83,    82,    11,    25,     7,    63,    64,    65,    66,
      67,    95,    97,    98,    38,     8,    39,     9,    10,    72,
      73,    74,    86,    87,    75,    76,    11,    12,     8,    13,
      45,    46,    47,    14,    40,    41,   102,     6,    42,    11,
      25,    29,    52,    54,     7,    88,    89,    49,    50,   -23,
      40,     8,    40,    40,     8,    61,     9,    10,    26,    27,
       6,    31,    11,    25,    59,    11,    12,     7,    13,    55,
      68,   101,    14,    69,    62,    78,    70,     8,    77,     9,
      10,    84,    43,    44,    45,    46,    47,    85,    11,    12,
      51,    13,    90,   100,    33,    14,    56,    57,    58,    43,
      44,    45,    46,    47,    34,    11,    25,    43,    44,    45,
      46,    47,    94,    56,    57,    58,    43,    44,    45,    46,
      47,    99,    48
};

static const yytype_int8 yycheck[] =
{
      15,     8,     9,    10,    15,    16,    17,    18,    19,    15,
      16,    17,    18,    19,     3,     4,    15,    16,    17,    18,
      19,    71,    22,    20,    23,    34,    33,    34,     5,     0,
      37,    42,    38,    33,    34,    12,    43,    44,    45,    46,
      47,    91,    92,    93,    34,    22,    34,    24,    25,    56,
      57,    58,    27,    28,    61,    62,    33,    34,    22,    36,
      17,    18,    19,    40,    79,    21,    43,     5,    21,    33,
      34,    35,    33,    34,    12,    82,    83,     6,    11,    21,
      95,    22,    97,    98,    22,    37,    24,    25,     6,     7,
       5,     9,    33,    34,    35,    33,    34,    12,    36,    26,
       7,    39,    40,    13,    41,    33,    23,    22,     8,    24,
      25,     7,    15,    16,    17,    18,    19,    14,    33,    34,
      23,    36,     9,    28,    22,    40,    29,    30,    31,    15,
      16,    17,    18,    19,    32,    33,    34,    15,    16,    17,
      18,    19,    10,    29,    30,    31,    15,    16,    17,    18,
      19,    33,    21
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,    45,    46,     0,     5,    12,    22,    24,
      25,    33,    34,    36,    40,    47,    48,    49,    50,    51,
      52,    54,    61,    62,    63,    34,    61,    61,    62,    35,
      53,    61,    62,    22,    32,    60,    62,    20,    34,    34,
      48,    21,    21,    15,    16,    17,    18,    19,    21,     6,
      11,    23,    60,    62,    60,    26,    29,    30,    31,    35,
      62,    37,    41,    62,    62,    62,    62,    62,     7,    13,
      23,    55,    62,    62,    62,    62,    62,     8,    33,    47,
      56,    58,    38,    42,     7,    14,    27,    28,    62,    62,
       9,    57,    64,    65,    10,    47,    59,    47,    47,    33,
      28,    39,    43
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    44,    45,    46,    46,    46,    47,    47,    47,    48,
      48,    48,    48,    48,    49,    49,    49,    49,    50,    51,
      52,    53,    53,    53,    55,    54,    56,    57,    56,    58,
      59,    60,    60,    60,    60,    60,    60,    61,    62,    62,
      62,    62,    62,    62,    62,    62,    64,    63,    65,    63
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     0,     2,     1,     0,     2,
       2,     2,     1,     1,     1,     1,     3,     3,     9,     6,
       2,     1,     1,     1,     0,     5,     2,     0,     5,     1,
       1,     3,     3,     3,     2,     3,     1,     1,     1,     1,
       3,     3,     3,     3,     3,     3,     0,     9,     0,     9
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: mood_declaration statements  */
#line 226 "vibeparser.y"
                                {
        if (!skip_until_endif) {
            finalize_llvm("output.ll");
        }
    }
#line 1392 "vibeparser.tab.c"
    break;

  case 3: /* mood_declaration: MOOD_SARCASTIC  */
#line 234 "vibeparser.y"
                   { current_mood = SARCASTIC; }
#line 1398 "vibeparser.tab.c"
    break;

  case 4: /* mood_declaration: MOOD_ROMANTIC  */
#line 235 "vibeparser.y"
                    { current_mood = ROMANTIC; }
#line 1404 "vibeparser.tab.c"
    break;

  case 5: /* mood_declaration: %empty  */
#line 236 "vibeparser.y"
                  { current_mood = NEUTRAL; }
#line 1410 "vibeparser.tab.c"
    break;

  case 10: /* statement: expression SEMICOLON  */
#line 247 "vibeparser.y"
                           { 
        if (current_mood != SARCASTIC && current_mood != ROMANTIC && !skip_until_endif) {
            printf("Expression result: %d\n", (yyvsp[-1].expr_result).value); 
        }
    }
#line 1420 "vibeparser.tab.c"
    break;

  case 14: /* variable_decl: romantic_decl  */
#line 258 "vibeparser.y"
                  { if (!skip_until_endif) { printf("ROMANTIC DECL: %s\n", (yyvsp[0].str)); free((yyvsp[0].str)); } }
#line 1426 "vibeparser.tab.c"
    break;

  case 15: /* variable_decl: sarcastic_decl  */
#line 259 "vibeparser.y"
                     { if (!skip_until_endif) { printf("SARCASTIC DECL: %s\n", (yyvsp[0].str)); free((yyvsp[0].str)); } }
#line 1432 "vibeparser.tab.c"
    break;

  case 16: /* variable_decl: IDENTIFIER ASSIGN expression  */
#line 260 "vibeparser.y"
                                   { 
        if (!skip_until_endif) { 
            add_symbol((yyvsp[-2].str), (yyvsp[0].expr_result).value); 
            free((yyvsp[-2].str)); 
        } 
    }
#line 1443 "vibeparser.tab.c"
    break;

  case 17: /* variable_decl: IDENTIFIER ASSIGN STRING_LITERAL  */
#line 266 "vibeparser.y"
                                       { 
        if (!skip_until_endif) { 
            add_string_symbol((yyvsp[-2].str), (yyvsp[0].str)); 
            free((yyvsp[-2].str)); 
            free((yyvsp[0].str)); 
        } 
    }
#line 1455 "vibeparser.tab.c"
    break;

  case 18: /* romantic_decl: LET identifier_term BE AS RADIANT AS THE NUMBER INTEGER  */
#line 276 "vibeparser.y"
                                                            {
        (yyval.str) = malloc(100);
        sprintf((yyval.str), "int %s = %d;", (yyvsp[-7].str), (yyvsp[0].num));
        if (!skip_until_endif) {
            add_symbol((yyvsp[-7].str), (yyvsp[0].num));
        }
        free((yyvsp[-7].str));
    }
#line 1468 "vibeparser.tab.c"
    break;

  case 19: /* sarcastic_decl: SARCASTIC_WOW identifier_term IS SARCASTIC_NOW INTEGER SARCASTIC_REV  */
#line 287 "vibeparser.y"
                                                                         {
        (yyval.str) = malloc(100);
        sprintf((yyval.str), "int %s = %d; // Revolutionary", (yyvsp[-4].str), (yyvsp[-1].num));
        if (!skip_until_endif) {
            add_symbol((yyvsp[-4].str), (yyvsp[-1].num));
        }
        free((yyvsp[-4].str));
    }
#line 1481 "vibeparser.tab.c"
    break;

  case 20: /* print_statement: PRINT print_value  */
#line 298 "vibeparser.y"
                      {
        if (!skip_until_endif) {
            if (current_mood == SARCASTIC) {
                printf("Oh wow Mr coder, here's your output Mr Perfect Wanna Be: %s", (yyvsp[0].str));
            } else if (current_mood == ROMANTIC) {
                printf("❤️ softly romantically and quietly whispers * %s", (yyvsp[0].str));
            } else {
                printf("Output: %s", (yyvsp[0].str));
            }
            printf("\n");
            
            // Generate LLVM IR for print statement
            LLVMValueRef print_val = NULL;
            int is_string = 0;
            
            // Check if it's a string literal or variable
            for (int i = 0; i < sym_count; i++) {
                if (symbol_table[i].name && strcmp(symbol_table[i].name, (yyvsp[0].str)) == 0) {
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
                long val = strtol((yyvsp[0].str), &endptr, 10);
                if (*endptr == '\0') {
                    // It's an integer literal
                    print_val = LLVMConstInt(LLVMInt32Type(), val, 0);
                    is_string = 0;
                } else {
                    // It's a string literal
                    print_val = LLVMBuildGlobalStringPtr(builder, (yyvsp[0].str), "str_print");
                    is_string = 1;
                }
            }
            
            gen_print(print_val, is_string);
        }
        free((yyvsp[0].str));
    }
#line 1536 "vibeparser.tab.c"
    break;

  case 21: /* print_value: expression  */
#line 351 "vibeparser.y"
               {
        char buffer[32];
        sprintf(buffer, "%d", (yyvsp[0].expr_result).value);
        (yyval.str) = strdup(buffer);
    }
#line 1546 "vibeparser.tab.c"
    break;

  case 22: /* print_value: STRING_LITERAL  */
#line 356 "vibeparser.y"
                     { (yyval.str) = (yyvsp[0].str); }
#line 1552 "vibeparser.tab.c"
    break;

  case 23: /* print_value: identifier_term  */
#line 357 "vibeparser.y"
                      {
        if (is_string_symbol((yyvsp[0].str))) {
            (yyval.str) = get_symbol_string((yyvsp[0].str));
        } else {
            char buffer[32];
            sprintf(buffer, "%d", get_symbol_value((yyvsp[0].str)));
            (yyval.str) = strdup(buffer);
        }
        free((yyvsp[0].str));
    }
#line 1567 "vibeparser.tab.c"
    break;

  case 24: /* $@1: %empty  */
#line 370 "vibeparser.y"
                      {
        condition_result = (yyvsp[-1].expr_result).value;
        if (!condition_result) {
            skip_until_endif = 1;
        }
        
        // Generate LLVM IR for if statement
        if (!skip_until_endif && (yyvsp[-1].expr_result).llvm_value) {
            start_if_statement((yyvsp[-1].expr_result).llvm_value);
        }
    }
#line 1583 "vibeparser.tab.c"
    break;

  case 25: /* if_statement: IF condition THEN $@1 if_block  */
#line 380 "vibeparser.y"
               {
        skip_until_endif = 0;
        (yyval.num) = (yyvsp[0].num);
    }
#line 1592 "vibeparser.tab.c"
    break;

  case 26: /* if_block: if_true_block ENDIF  */
#line 387 "vibeparser.y"
                        { 
        (yyval.num) = condition_result; 
        // End if statement in LLVM IR
        if (!skip_until_endif) {
            end_if_statement();
        }
    }
#line 1604 "vibeparser.tab.c"
    break;

  case 27: /* $@2: %empty  */
#line 394 "vibeparser.y"
                         {
        if (condition_result) {
            skip_until_endif = 1;
        } else {
            skip_until_endif = 0;
        }
        
        // Generate LLVM IR for else part
        if (!skip_until_endif) {
            handle_else();
        }
    }
#line 1621 "vibeparser.tab.c"
    break;

  case 28: /* if_block: if_true_block ELSE $@2 if_false_block ENDIF  */
#line 405 "vibeparser.y"
                           {
        skip_until_endif = 0;
        (yyval.num) = condition_result;
        
        // End if statement in LLVM IR
        if (!skip_until_endif) {
            end_if_statement();
        }
    }
#line 1635 "vibeparser.tab.c"
    break;

  case 29: /* if_true_block: statements  */
#line 417 "vibeparser.y"
               { (yyval.num) = condition_result; }
#line 1641 "vibeparser.tab.c"
    break;

  case 30: /* if_false_block: statements  */
#line 421 "vibeparser.y"
               { (yyval.num) = condition_result; }
#line 1647 "vibeparser.tab.c"
    break;

  case 31: /* condition: expression EQ expression  */
#line 425 "vibeparser.y"
                             { 
        int result = ((yyvsp[-2].expr_result).value == (yyvsp[0].expr_result).value);
        (yyval.expr_result).value = result;
        if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, EQ, (yyvsp[0].expr_result).llvm_value);
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1661 "vibeparser.tab.c"
    break;

  case 32: /* condition: expression GT expression  */
#line 434 "vibeparser.y"
                               { 
        int result = ((yyvsp[-2].expr_result).value > (yyvsp[0].expr_result).value);
        (yyval.expr_result).value = result;
        if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, GT, (yyvsp[0].expr_result).llvm_value);
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1675 "vibeparser.tab.c"
    break;

  case 33: /* condition: expression LT expression  */
#line 443 "vibeparser.y"
                               { 
        int result = ((yyvsp[-2].expr_result).value < (yyvsp[0].expr_result).value);
        (yyval.expr_result).value = result;
        if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, LT, (yyvsp[0].expr_result).llvm_value);
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1689 "vibeparser.tab.c"
    break;

  case 34: /* condition: NOT condition  */
#line 452 "vibeparser.y"
                    { 
        (yyval.expr_result).value = !(yyvsp[0].expr_result).value;
        if (!skip_until_endif && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = LLVMBuildNot(builder, (yyvsp[0].expr_result).llvm_value, "nottmp");
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1702 "vibeparser.tab.c"
    break;

  case 35: /* condition: LPAREN condition RPAREN  */
#line 460 "vibeparser.y"
                              { (yyval.expr_result) = (yyvsp[-1].expr_result); }
#line 1708 "vibeparser.tab.c"
    break;

  case 36: /* condition: expression  */
#line 461 "vibeparser.y"
                 { 
        (yyval.expr_result).value = (yyvsp[0].expr_result).value != 0;
        if (!skip_until_endif && (yyvsp[0].expr_result).llvm_value) {
            LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);
            (yyval.expr_result).llvm_value = LLVMBuildICmp(builder, LLVMIntNE, (yyvsp[0].expr_result).llvm_value, zero, "condtmp");
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1722 "vibeparser.tab.c"
    break;

  case 37: /* identifier_term: IDENTIFIER  */
#line 473 "vibeparser.y"
               { (yyval.str) = (yyvsp[0].str); }
#line 1728 "vibeparser.tab.c"
    break;

  case 38: /* expression: INTEGER  */
#line 477 "vibeparser.y"
            { 
        (yyval.expr_result).value = (yyvsp[0].num);
        if (!skip_until_endif) {
            (yyval.expr_result).llvm_value = gen_expression((yyvsp[0].num));
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1741 "vibeparser.tab.c"
    break;

  case 39: /* expression: identifier_term  */
#line 485 "vibeparser.y"
                      { 
        (yyval.expr_result).value = get_symbol_value((yyvsp[0].str));
        if (!skip_until_endif) {
            LLVMValueRef sym_val = get_symbol_value_llvm((yyvsp[0].str));
            if (sym_val) {
                (yyval.expr_result).llvm_value = LLVMBuildLoad2(builder, LLVMInt32Type(), sym_val, "varload");
            } else {
                (yyval.expr_result).llvm_value = NULL;
            }
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
        free((yyvsp[0].str));
    }
#line 1760 "vibeparser.tab.c"
    break;

  case 40: /* expression: expression PLUS expression  */
#line 499 "vibeparser.y"
                                 { 
        (yyval.expr_result).value = (yyvsp[-2].expr_result).value + (yyvsp[0].expr_result).value;
        if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, PLUS, (yyvsp[0].expr_result).llvm_value);
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1773 "vibeparser.tab.c"
    break;

  case 41: /* expression: expression MINUS expression  */
#line 507 "vibeparser.y"
                                  { 
        (yyval.expr_result).value = (yyvsp[-2].expr_result).value - (yyvsp[0].expr_result).value;
        if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, MINUS, (yyvsp[0].expr_result).llvm_value);
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1786 "vibeparser.tab.c"
    break;

  case 42: /* expression: expression TIMES expression  */
#line 515 "vibeparser.y"
                                  { 
        (yyval.expr_result).value = (yyvsp[-2].expr_result).value * (yyvsp[0].expr_result).value;
        if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
            (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, TIMES, (yyvsp[0].expr_result).llvm_value);
        } else {
            (yyval.expr_result).llvm_value = NULL;
        }
    }
#line 1799 "vibeparser.tab.c"
    break;

  case 43: /* expression: expression DIVIDE expression  */
#line 523 "vibeparser.y"
                                   { 
        if ((yyvsp[0].expr_result).value == 0) {
            yyerror("Division by zero");
            (yyval.expr_result).value = 0;
            (yyval.expr_result).llvm_value = NULL;
        } else {
            (yyval.expr_result).value = (yyvsp[-2].expr_result).value / (yyvsp[0].expr_result).value;
            if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
                (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, DIVIDE, (yyvsp[0].expr_result).llvm_value);
            } else {
                (yyval.expr_result).llvm_value = NULL;
            }
        }
    }
#line 1818 "vibeparser.tab.c"
    break;

  case 44: /* expression: expression MOD expression  */
#line 537 "vibeparser.y"
                                {
        if ((yyvsp[0].expr_result).value == 0) {
            yyerror("Modulo by zero");
            (yyval.expr_result).value = 0;
            (yyval.expr_result).llvm_value = NULL;
        } else {
            (yyval.expr_result).value = (yyvsp[-2].expr_result).value % (yyvsp[0].expr_result).value;
            if (!skip_until_endif && (yyvsp[-2].expr_result).llvm_value && (yyvsp[0].expr_result).llvm_value) {
                (yyval.expr_result).llvm_value = gen_binary_op((yyvsp[-2].expr_result).llvm_value, MOD, (yyvsp[0].expr_result).llvm_value);
            } else {
                (yyval.expr_result).llvm_value = NULL;
            }
        }
    }
#line 1837 "vibeparser.tab.c"
    break;

  case 45: /* expression: LPAREN expression RPAREN  */
#line 551 "vibeparser.y"
                               { (yyval.expr_result) = (yyvsp[-1].expr_result); }
#line 1843 "vibeparser.tab.c"
    break;

  case 46: /* $@3: %empty  */
#line 555 "vibeparser.y"
                                                                                  {
        if (!skip_until_endif) {
            // Initialize loop
            current_loop.loop_var = strdup((yyvsp[-4].str));
            current_loop.start_value = (yyvsp[-2].expr_result).value;
            current_loop.end_value = (yyvsp[0].expr_result).value;
            current_loop.current_value = current_loop.start_value;
            in_loop = 1;
            
            // Add loop variable to symbol table
            add_symbol((yyvsp[-4].str), current_loop.current_value);
            
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
        free((yyvsp[-4].str));
    }
#line 1881 "vibeparser.tab.c"
    break;

  case 47: /* for_statement: SARCASTIC_FOR IDENTIFIER SARCASTIC_WITH expression SARCASTIC_UNTIL expression $@3 statements SARCASTIC_NEXT  */
#line 587 "vibeparser.y"
                                {
        if (!skip_until_endif && current_mood == SARCASTIC) {
            printf("Wow, that loop is finally over. Shocking.\n");
        }
        (yyval.num) = 1;
    }
#line 1892 "vibeparser.tab.c"
    break;

  case 48: /* $@4: %empty  */
#line 593 "vibeparser.y"
                                                                                 {
        if (!skip_until_endif) {
            // Initialize loop
            current_loop.loop_var = strdup((yyvsp[-4].str));
            current_loop.start_value = (yyvsp[-2].expr_result).value;
            current_loop.end_value = (yyvsp[0].expr_result).value;
            current_loop.current_value = current_loop.start_value;
            in_loop = 1;
            
            // Add loop variable to symbol table
            add_symbol((yyvsp[-4].str), current_loop.current_value);
            
            if (current_mood == ROMANTIC) {
                printf("Let us dance together, %s, from %d until %d, like lovers under the moonlight.\n", 
                       (yyvsp[-4].str), current_loop.start_value, current_loop.end_value);
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
        free((yyvsp[-4].str));
    }
#line 1929 "vibeparser.tab.c"
    break;

  case 49: /* for_statement: ROMANTIC_FOR IDENTIFIER ROMANTIC_WITH expression ROMANTIC_UNTIL expression $@4 statements ROMANTIC_NEXT  */
#line 624 "vibeparser.y"
                               {
        if (!skip_until_endif && current_mood == ROMANTIC) {
            printf("Our dance comes to an end, like a gentle sunset.\n");
        }
        (yyval.num) = 1;
    }
#line 1940 "vibeparser.tab.c"
    break;


#line 1944 "vibeparser.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 632 "vibeparser.y"


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
