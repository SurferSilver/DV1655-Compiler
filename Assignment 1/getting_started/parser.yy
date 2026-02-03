/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Skeleton and definitions for generating a LALR(1) parser in C++ */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

%skeleton "lalr1.cc" 
%defines
%define parse.error verbose
%define api.value.type variant
%define api.token.constructor

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Required code included before the parser definition begins */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

%code requires{
  #include <string>
  #include "Node.h"
  #define USE_LEX_ONLY false //change this macro to true if you want to isolate the lexer from the parser.
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Code included in the parser implementation file */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

%code{
  #define YY_DECL yy::parser::symbol_type yylex()
  YY_DECL;
  
  Node* root;
  extern int yylineno;
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Token definitions for the grammar */
/* Tokens represent the smallest units of the language, like operators and parentheses */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Identifiers */
%token <std::string>INT
%token <std::string>ID
%token <std::string>TYPE_INT
%token <std::string>TYPE_FLOAT
%token <std::string>ASSIGNOP
%token <std::string>MAIN
%token <std::string>VOLATILE
%token <std::string>PRINT
%token <std::string>RETURN
%token <std::string>NEWLINE

/* Parentheses and braces */
%token <std::string>LP
%token <std::string>RP
%token <std::string>LBRACK
%token <std::string>RBRACK
%token <std::string>LBRACE
%token <std::string>RBRACE

/* Punctuation */
%token <std::string>COMMA
%token <std::string>SEMICOLON
%token <std::string>COLON

/* Math Operators */
%token <std::string>PLUSOP
%token <std::string>MINUSOP
%token <std::string>MULTOP
%token <std::string>DIVOP
%token <std::string>FLOAT
%token <std::string>EXPOP


%token END 0 "end of file"

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Operator precedence and associativity rules */
/* Used to resolve ambiguities in parsing expressions See https://www.gnu.org/software/bison/manual/bison.html#Precedence-Decl */ 
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

%left PLUSOP MINUSOP
%left MULTOP DIVOP
%left EXPOP
%right ASSIGNOP


/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Specify types for non-terminals in the grammar */
/* The type specifies the data type of the values associated with these non-terminals */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

%type <Node *> root 
%type <Node *> expression 
%type <Node *> factor 
%type <Node *> statement
%type <Node *> program
%type <Node *> entry
%type <Node *> type
%type <Node *> var
%type <Node *> OPTIONAL_NEWLINE

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Grammar rules section */
/* This section defines the production rules for the language being parsed */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
%%
root:
    entry END   { root = $1; }
  | program END { root = $1; }
  ;

entry:
    MAIN LP RP COLON type LBRACE program RBRACE
      { 
        $$ = new Node("Entry","", yylineno);
        $$->children.push_back($7);
      }
  ;

program:
    expression                { $$ = new Node("Program","", yylineno); $$->children.push_back($1); }
  | statement                 { $$ = new Node("Program","", yylineno); $$->children.push_back($1); }
  | program expression        { $$ = $1; $$->children.push_back($2); }
  | program statement         { $$ = $1; $$->children.push_back($2); }
  ;

type:
    TYPE_FLOAT                                { $$ = new Node("Type", $1, yylineno); }
  | TYPE_INT                                  { $$ = new Node("Type", $1, yylineno); }
 ;


/*
- expression
  Use when the construct produces a value and may contain operators.
  Top-level evaluatable syntax.
  Delegates to lower-precedence layers.
*/
expression: expression PLUSOP expression 
            {      
              /*Create a subtree that corresponds to the AddExpression
                The root of the subtree is AddExpression
                The childdren of the AddExpression subtree are the left hand side (expression accessed through $1) 
                and right hand side of the expression (expression accessed through $3)
              */
              $$ = new Node("AddExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
              /* printf("r1 "); */
            }
            | expression MINUSOP expression 
            {
              $$ = new Node("SubExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
              /* printf("r2 "); */
            }
            | expression MULTOP expression 
            {
              $$ = new Node("MultExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
              /* printf("r3 "); */
            }
            | expression DIVOP expression 
            {
              $$ = new Node("DivExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
              /* printf("r4 "); */
            }
            | expression EXPOP expression 
            {
              $$ = new Node("ExpExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
              /* printf("r9 "); */
            }
            | factor      
            {
            $$ = $1; 
            /* printf("r5 ");*/
            }
            ;

/*
- factor / primary
  Use for atomic operands and grouping.
  No operators.
  Recursion stops here.
  Literals, identifiers, parenthesized expressions.
*/
factor:     INT           
              { 
              $$ = new Node("Int", $1, yylineno);
              /* printf("r6 ");  Here we create a leaf node Int. The value of the leaf node is $1 */
              }
            | FLOAT         
              {
              $$ = new Node("Float", $1, yylineno);
              /* printf("r11 ");  Here we create a leaf node Float. The value of the leaf node is $1 */
              }
            | LP expression RP 
              {
              $$ = $2;
              /* printf("r7 ");  simply return the expression */
              }
            | LBRACE expression RBRACE 
              {
              $$ = $2;
              /* printf("r10 ");  simply return the expression */
              }  
            | ID            
              {
              $$ = new Node("Id", $1, yylineno);
              }
            | END           
              { 
              /* printf("r8 "); don't really know what to do here */
              }
          ;
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Additional helpers for statements and other constructs */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

OPTIONAL_NEWLINE:
    NEWLINE    { $$ = new Node("Newline", $1, yylineno); }
  |             { $$ = new Node("NoNewline", "", yylineno); }
  ;

var:
  VOLATILE ID COLON type
  {
    $$ = new Node("VarDecl","",yylineno);
    $$->children.push_back(new Node("Volatile","",yylineno));
    $$->children.push_back(new Node("ID",$2,yylineno));
    $$->children.push_back($4);
  }
  | ID COLON type 
    {
      $$ = new Node("VarDecl","",yylineno);
      $$->children.push_back(new Node("ID",$1,yylineno));
      $$->children.push_back($3);
    }
  ;

/*
- Statements
  Use for constructs that do not produce a value.
  Top-level executable syntax.
*/
statement:  var ASSIGNOP expression OPTIONAL_NEWLINE
            {
              $$ = new Node("AssignStatement", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back(new Node("Id", $2, yylineno));
              $$->children.push_back($3);
              $$->children.push_back($4);
            }
          | PRINT LP ID RP
            {
              $$ = new Node("PrintStatement", "", yylineno);
              $$->children.push_back(new Node("Id", $3, yylineno));
            }
          | RETURN expression
            {
              $$ = new Node("ReturnStatement", "", yylineno);
              $$->children.push_back($2);
            }
          ;
%%

