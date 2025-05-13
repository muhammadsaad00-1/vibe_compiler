/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_VIBEPARSER_TAB_H_INCLUDED
# define YY_YY_VIBEPARSER_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 188 "vibeparser.y"

    #include "vibeparser_llvm.h"

#line 53 "vibeparser.tab.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    MOOD_SARCASTIC = 258,          /* MOOD_SARCASTIC  */
    MOOD_ROMANTIC = 259,           /* MOOD_ROMANTIC  */
    LET = 260,                     /* LET  */
    BE = 261,                      /* BE  */
    AS = 262,                      /* AS  */
    RADIANT = 263,                 /* RADIANT  */
    THE = 264,                     /* THE  */
    NUMBER = 265,                  /* NUMBER  */
    IS = 266,                      /* IS  */
    SARCASTIC_WOW = 267,           /* SARCASTIC_WOW  */
    SARCASTIC_NOW = 268,           /* SARCASTIC_NOW  */
    SARCASTIC_REV = 269,           /* SARCASTIC_REV  */
    PLUS = 270,                    /* PLUS  */
    MINUS = 271,                   /* MINUS  */
    TIMES = 272,                   /* TIMES  */
    DIVIDE = 273,                  /* DIVIDE  */
    MOD = 274,                     /* MOD  */
    ASSIGN = 275,                  /* ASSIGN  */
    SEMICOLON = 276,               /* SEMICOLON  */
    LPAREN = 277,                  /* LPAREN  */
    RPAREN = 278,                  /* RPAREN  */
    PRINT = 279,                   /* PRINT  */
    IF = 280,                      /* IF  */
    THEN = 281,                    /* THEN  */
    ELSE = 282,                    /* ELSE  */
    ENDIF = 283,                   /* ENDIF  */
    EQ = 284,                      /* EQ  */
    GT = 285,                      /* GT  */
    LT = 286,                      /* LT  */
    NOT = 287,                     /* NOT  */
    INTEGER = 288,                 /* INTEGER  */
    IDENTIFIER = 289,              /* IDENTIFIER  */
    STRING_LITERAL = 290,          /* STRING_LITERAL  */
    SARCASTIC_FOR = 291,           /* SARCASTIC_FOR  */
    SARCASTIC_WITH = 292,          /* SARCASTIC_WITH  */
    SARCASTIC_UNTIL = 293,         /* SARCASTIC_UNTIL  */
    SARCASTIC_NEXT = 294,          /* SARCASTIC_NEXT  */
    ROMANTIC_FOR = 295,            /* ROMANTIC_FOR  */
    ROMANTIC_WITH = 296,           /* ROMANTIC_WITH  */
    ROMANTIC_UNTIL = 297,          /* ROMANTIC_UNTIL  */
    ROMANTIC_NEXT = 298            /* ROMANTIC_NEXT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 192 "vibeparser.y"

    int num;
    char *str;
    ExprResult expr_result;

#line 119 "vibeparser.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_VIBEPARSER_TAB_H_INCLUDED  */
