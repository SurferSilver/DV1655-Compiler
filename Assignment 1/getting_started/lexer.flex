%top{
    #include "parser.tab.hh"
    #define YY_DECL yy::parser::symbol_type yylex()
    #include "Node.h"
    int lexical_errors = 0;
}
%option yylineno noyywrap nounput batch noinput stack 
%%

"main"                 {if(USE_LEX_ONLY) {printf("MAIN ");} else {return yy::parser::make_MAIN(yytext);}}
"volatile"             {if(USE_LEX_ONLY) {printf("VOLATILE ");} else {return yy::parser::make_VOLATILE(yytext);}}
"return"               {if(USE_LEX_ONLY) {printf("RETURN ");} else {return yy::parser::make_RETURN(yytext);}}
"for"                  {if(USE_LEX_ONLY) {printf("FOR ");} else {return yy::parser::make_FOR(yytext);}}
"if"                   {if(USE_LEX_ONLY) {printf("IF ");} else {return yy::parser::make_IF(yytext);}}
"else"                 {if(USE_LEX_ONLY) {printf("ELSE ");} else {return yy::parser::make_ELSE(yytext);}}
"int"                  {if(USE_LEX_ONLY) {printf("TYPE_INT ");} else {return yy::parser::make_TYPE_INT(yytext);}}
"float"                {if(USE_LEX_ONLY) {printf("TYPE_FLOAT ");} else {return yy::parser::make_TYPE_FLOAT(yytext);}}
"length"                {if(USE_LEX_ONLY) {printf("LENGTH ");} else {return yy::parser::make_LENGTH(yytext);}}

"("                     {if(USE_LEX_ONLY) {printf("LP ");} else {return yy::parser::make_LP(yytext);}}
")"                     {if(USE_LEX_ONLY) {printf("RP ");} else {return yy::parser::make_RP(yytext);}}
"["                     {if(USE_LEX_ONLY) {printf("LBRACK ");} else {return yy::parser::make_LBRACK(yytext);}}
"]"                     {if(USE_LEX_ONLY) {printf("RBRACK ");} else {return yy::parser::make_RBRACK(yytext);}}
"{"                     {if(USE_LEX_ONLY) {printf("LBRACE ");} else {return yy::parser::make_LBRACE(yytext);}}
"}"                     {if(USE_LEX_ONLY) {printf("RBRACE ");} else {return yy::parser::make_RBRACE(yytext);}}

"."                    {if(USE_LEX_ONLY) {printf("DOT ");} else {return yy::parser::make_DOT(yytext);}}
","                     {if(USE_LEX_ONLY) {printf("COMMA ");} else {return yy::parser::make_COMMA(yytext);}}
":"                     {if(USE_LEX_ONLY) {printf("COLON ");} else {return yy::parser::make_COLON(yytext);}}

"+"                     {if(USE_LEX_ONLY) {printf("PLUSOP ");} else {return yy::parser::make_PLUSOP(yytext);}}
"-"                     {if(USE_LEX_ONLY) {printf("MINUSOP ");} else {return yy::parser::make_MINUSOP(yytext);}}
"*"                     {if(USE_LEX_ONLY) {printf("MULTOP ");} else {return yy::parser::make_MULTOP(yytext);}}
"/"                     {if(USE_LEX_ONLY) {printf("DIVOP ");} else {return yy::parser::make_DIVOP(yytext);}}
"^"                     {if(USE_LEX_ONLY) {printf("EXPOP ");} else {return yy::parser::make_EXPOP(yytext);}}
":="                    {if(USE_LEX_ONLY) {printf("ASSIGNOP ");} else {return yy::parser::make_ASSIGNOP(yytext);}}

"<="                    {if(USE_LEX_ONLY) {printf("LEQOP ");} else {return yy::parser::make_LEQOP(yytext);}}
">="                    {if(USE_LEX_ONLY) {printf("GEQOP ");} else {return yy::parser::make_GEQOP(yytext);}}
"<"                     {if(USE_LEX_ONLY) {printf("LTOP ");} else {return yy::parser::make_LTOP(yytext);}}
">"                     {if(USE_LEX_ONLY) {printf("GTOP ");} else {return yy::parser::make_GTOP(yytext);}}
"="                     {if(USE_LEX_ONLY) {printf("EQOP ");} else {return yy::parser::make_EQOP(yytext);}}
"!="                    {if(USE_LEX_ONLY) {printf("NEQOP ");} else {return yy::parser::make_NEQOP(yytext);}}
"&"                     {if(USE_LEX_ONLY) {printf("AND ");} else {return yy::parser::make_AND(yytext);}}
"|"                     {if(USE_LEX_ONLY) {printf("OR ");} else {return yy::parser::make_OR(yytext);}}

"print"                {if(USE_LEX_ONLY) {printf("PRINT ");} else {return yy::parser::make_PRINT(yytext);}}
"read"                 {if(USE_LEX_ONLY) {printf("READ ");} else {return yy::parser::make_READ(yytext);}}

0|[1-9][0-9]*           {if(USE_LEX_ONLY) {printf("INT ");} else {return yy::parser::make_INT(yytext);}}
[0-9]+"."[0-9]*         {if (USE_LEX_ONLY) {printf("FLOAT ");} else {return yy::parser::make_FLOAT(yytext);}}
["a-zA-Z_"]["a-zA-Z0-9_"]*   {if(USE_LEX_ONLY) {printf("ID ");} else {return yy::parser::make_ID(yytext);}}

[ \t\n\r]+              {}
"//"[^\n]*              {}

.                       { if(!lexical_errors) fprintf(stderr, "Lexical errors found! See the logs below: \n"); fprintf(stderr,"\t@error at line %d. Character %s is not recognized\n", yylineno, yytext); lexical_errors = 1;}
<<EOF>>                  {return yy::parser::make_END();}
%%