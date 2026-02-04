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
%token <std::string>FLOAT
%token <std::string>TYPE_INT
%token <std::string>TYPE_FLOAT
%token <std::string>ASSIGNOP
%token <std::string>MAIN
%token <std::string>VOLATILE
%token <std::string>RETURN
%token <std::string>FOR
%token <std::string>IF
%token <std::string>ELSE
%token <std::string>LENGTH

/* Parentheses and braces */
%token <std::string>LP
%token <std::string>RP
%token <std::string>LBRACK
%token <std::string>RBRACK
%token <std::string>LBRACE
%token <std::string>RBRACE

/* Punctuation */
%token <std::string>COMMA
%token <std::string>COLON
%token <std::string>DOT

/* Math Operators */
%token <std::string>PLUSOP
%token <std::string>MINUSOP
%token <std::string>MULTOP
%token <std::string>DIVOP
%token <std::string>EXPOP

/* Relational Operators */
%token <std::string>LEQOP
%token <std::string>GEQOP
%token <std::string>LTOP
%token <std::string>GTOP
%token <std::string>EQOP
%token <std::string>NEQOP
%token <std::string>AND
%token <std::string>OR

/* Functions */
%token <std::string>READ
%token <std::string>PRINT

%token END 0 "end of file"

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Operator precedence and associativity rules */
/* Used to resolve ambiguities in parsing expressions See https://www.gnu.org/software/bison/manual/bison.html#Precedence-Decl */ 
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */


%left AND OR
%nonassoc LEQOP GEQOP LTOP GTOP EQOP NEQOP
%left PLUSOP MINUSOP
%left MULTOP DIVOP
%left EXPOP
%left LENGTH
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
%type <Node *> program_block
%type <Node *> entry
%type <Node *> type
%type <Node *> var
%type <Node *> list_content
%type <Node *> base_type

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
    MAIN LP RP COLON type program_block
      { 
        $$ = $6;
      }
  ;

program_block:
    LBRACE program RBRACE
      {
        $$ = $2;
      }
  ;

program:
    expression                { $$ = new Node("Program","", yylineno); $$->children.push_back($1); }
  | statement                 { $$ = new Node("Program","", yylineno); $$->children.push_back($1); }
  | program expression        { $$ = $1; $$->children.push_back($2); }
  | program statement         { $$ = $1; $$->children.push_back($2); }
  ;

 base_type:
    TYPE_INT                                { $$ = new Node("Type", $1, yylineno); }
  | TYPE_FLOAT                              { $$ = new Node("Type", $1, yylineno); }
;

type:
    base_type
      {
        $$ = new Node("Type", $1->value, yylineno);
      }
  | base_type LBRACK RBRACK
      {
        $$ = new Node("Type", $1->value + "[]", yylineno);
      }
  | ID
      {
        $$ = new Node("Type", $1, yylineno);
      }
  ;

/*------------------------------------------------------------------------------------------------------------------------------------------------- */
/* expression
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/
expression: expression PLUSOP expression 
            {      
              $$ = new Node("AddExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression MINUSOP expression
            {
              $$ = new Node("SubExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression MULTOP expression 
            {
              $$ = new Node("MultExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression DIVOP expression 
            {
              $$ = new Node("DivExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression EXPOP expression 
            {
              $$ = new Node("ExpExpression", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            |
            expression LEQOP expression
            {
              $$ = new Node("LeqExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression GEQOP expression
            {
              $$ = new Node("GeqExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression LTOP expression
            {
              $$ = new Node("LtExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression GTOP expression
            {
              $$ = new Node("GtExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression EQOP expression
            {
              $$ = new Node("EqExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression NEQOP expression
            {
              $$ = new Node("NeqExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression AND expression
            {
              $$ = new Node("AndExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | expression OR expression
            {
              $$ = new Node("OrExpression","",yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
            | ID LBRACK expression RBRACK
            {
              $$ = new Node("IndexExpression","",yylineno);
              $$->children.push_back(new Node("Id", $1, yylineno));
              $$->children.push_back($3);
            }
            | base_type LBRACK list_content RBRACK
              {
                $$ = new Node("ListExpression","",yylineno);
                $$->children.push_back(new Node("Type", $1->value, yylineno));
                $$->children.push_back($3);
            }
            | ID DOT LENGTH
            {
              $$ = new Node("LengthExpression","",yylineno);
              $$->children.push_back(new Node("Id", $1, yylineno));
            }
            | factor      
            {
              $$ = $1; 
            }
            ;

/*---------------------------------------------------------------------------------------------------------------------------------------------------- */
/* factor / primary
/*---------------------------------------------------------------------------------------------------------------------------------------------------- */

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
            | ID            
              {
              $$ = new Node("Id", $1, yylineno);
              }
          ;



/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Additional helpers for statements and other constructs */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

var:
  VOLATILE ID COLON type
  {
    $$ = new Node("Var","",yylineno);
    $$->children.push_back(new Node("Volatile","",yylineno));
    $$->children.push_back(new Node("Id",$2,yylineno));
    $$->children.push_back($4);
  }
  | ID COLON type
    {
      $$ = new Node("Var","",yylineno);
      $$->children.push_back(new Node("Id",$1,yylineno));
      $$->children.push_back($3);
    }
 ;

 list_content:
    expression
      {
        $$ = new Node("ListContent","", yylineno);
        $$->children.push_back($1);
      }
    | expression COMMA list_content
      {
        $$ = new Node("ListContent","", yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
      }
    ;


/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Statements
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

statement:  var ASSIGNOP expression
            {
              $$ = new Node("AssignStatement", "", yylineno);
              $$->children.push_back($1);
              $$->children.push_back($3);
            }
          | ID ASSIGNOP expression
            {
              $$ = new Node("AssignStatement", "", yylineno);
              $$->children.push_back(new Node("Id", $1, yylineno));
              $$->children.push_back($3);
            }
          | var
            {
              $$ = new Node("VarDeclaration", "", yylineno);
              $$->children.push_back($1);
            }
          | PRINT LP ID RP
            {
              $$ = new Node("PrintStatement", "", yylineno);
              $$->children.push_back(new Node("Id", $3, yylineno));
            }
          | READ LP ID RP
            {
              $$ = new Node("ReadStatement", "", yylineno);
              $$->children.push_back(new Node("Id", $3, yylineno));
            }
          | RETURN expression
            {
              $$ = new Node("ReturnStatement", "", yylineno);
              $$->children.push_back($2);
            }
          | FOR LP var COMMA expression COMMA ID ASSIGNOP expression RP program_block
            {
              $$ = new Node("ForStatement", "", yylineno);
              
              Node* start_condition = new Node("start_condition", "", yylineno);
              start_condition->children.push_back($3);
              $$->children.push_back(start_condition);
              
              Node* end_condition = new Node("end_condition", "", yylineno);
              end_condition->children.push_back($5);
              $$->children.push_back(end_condition);
              
              Node* step = new Node("step", "", yylineno);
              step->children.push_back(new Node("Id", $7, yylineno));
              step->children.push_back($9);
              $$->children.push_back(step);
              
              Node* end = new Node("end", "", yylineno);
              end->children.push_back($11);
              $$->children.push_back(end);
            }
          | FOR LP ID ASSIGNOP expression COMMA expression COMMA ID ASSIGNOP expression RP program_block
            {
              $$ = new Node("ForStatement", "", yylineno);
              
              Node* start_condition = new Node("start_condition", "", yylineno);
              start_condition->children.push_back(new Node("Id", $3, yylineno));
              start_condition->children.push_back($5);
              $$->children.push_back(start_condition);
              
              Node* end_condition = new Node("end_condition", "", yylineno);
              end_condition->children.push_back($7);
              $$->children.push_back(end_condition);
              
              Node* step = new Node("step", "", yylineno);
              step->children.push_back(new Node("Id", $9, yylineno));
              step->children.push_back($11);
              $$->children.push_back(step);
              
              Node* block = new Node("block", "", yylineno);
              block->children.push_back($13);
              $$->children.push_back(block);
            }
          | IF LP expression RP program_block
            {
              $$ = new Node("IfStatement", "", yylineno);
              
              Node* condition = new Node("condition", "", yylineno);
              condition->children.push_back($3);
              $$->children.push_back(condition);
              
              Node* then_branch = new Node("then_branch", "", yylineno);
              then_branch->children.push_back($5);
              $$->children.push_back(then_branch);
            }
          | IF LP expression RP program_block ELSE program_block
            {
              $$ = new Node("IfStatement", "", yylineno);
              
              Node* condition = new Node("condition", "", yylineno);
              condition->children.push_back($3);
              $$->children.push_back(condition);
              
              Node* then_branch = new Node("then_branch", "", yylineno);
              then_branch->children.push_back($5);
              $$->children.push_back(then_branch);
              
              Node* else_branch = new Node("else_branch", "", yylineno);
              else_branch->children.push_back($7);
              $$->children.push_back(else_branch);
            }
          ;
%%

