%option noyywrap
%option nounput
%option noinput
%option yylineno

%{
/**********************************************************************
 * LEXER
 **********************************************************************/

// Global settings, which will be added to flex' C source code

#include <cstdlib>
#include <string>

#include "sysy.tab.hpp"  // manifest constant from Bison header files

using namespace std;

%}

/* Regex definitions */

WhiteSpace      [ \t\n\r]*
LineComment     "//".*
BlockComment    "/*"([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+"/"

Identifier      [a-zA-Z_][a-zA-Z0-9_]*

Decimal         [1-9][0-9]*
Octal           0[0-7]*
Hexadecimal     0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* ignore */ }
{LineComment}   { /* ignore */ }
{BlockComment}  { /* ignore */ }

"int"           { return INT; }
"void"          { return VOID; }
"return"        { return RETURN; }
"const"         { return CONST; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

"<="            { yylval.str_val = new string(yytext); return LE; }
">="            { yylval.str_val = new string(yytext); return GE; }
"=="            { yylval.str_val = new string(yytext); return EQ; }
"!="            { yylval.str_val = new string(yytext); return NE; }
"&&"            { yylval.str_val = new string(yytext); return LAND; }
"||"            { yylval.str_val = new string(yytext); return LOR; }

.               { return yytext[0]; }

%%