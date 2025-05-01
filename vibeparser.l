%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mood.h"
#include "vibeparser.tab.h"

extern Mood current_mood;
void yyerror(const char *s);
%}

%option noyywrap
%option nounput
%option noinput

DIGIT    [0-9]
ID       [a-zA-Z][a-zA-Z0-9_]*
STRING   \"([^\"]|\\.)*\"

%%

"@mood(\"sarcastic\")"    { return MOOD_SARCASTIC; }
"@mood(\"romantic\")"     { return MOOD_ROMANTIC; }

"Let"                     { return LET; }
"be"                      { return BE; }
"as"                      { return AS; }
"radiant"                 { return RADIANT; }
"the"                     { return THE; }
"number"                  { return NUMBER; }
"is"                      { return IS; }

"Wow"                     { return SARCASTIC_WOW; }
"now"                     { return SARCASTIC_NOW; }
"Revolutionary"           { return SARCASTIC_REV; }

"Whisper"                 { return PRINT; }
"Shout"                   { return PRINT; }
"Proclaim"                { return PRINT; }

"If"                      { return IF; }
"Then"                    { return THEN; }
"Else"                    { return ELSE; }
"EndIf"                   { return ENDIF; }

"equals"                  { return EQ; }
"greater"                 { return GT; }
"less"                    { return LT; }
"not"                     { return NOT; }

"+"                       { return PLUS; }
"-"                       { return MINUS; }
"*"                       { return TIMES; }
"/"                       { return DIVIDE; }
"%"                       { return MOD; }
"="                       { return ASSIGN; }
";"                       { return SEMICOLON; }
"("                       { return LPAREN; }
")"                       { return RPAREN; }

{DIGIT}+                  { yylval.num = atoi(yytext); return INTEGER; }
{ID}                      { yylval.str = strdup(yytext); return IDENTIFIER; }
{STRING}                  { 
                            /* Strip the quotes from the string */
                            yytext[strlen(yytext)-1] = '\0';
                            yylval.str = strdup(yytext+1); 
                            return STRING_LITERAL; 
                          }

[ \t\n]                   ; /* skip whitespace */
.                         { yyerror("Invalid character"); }

%%