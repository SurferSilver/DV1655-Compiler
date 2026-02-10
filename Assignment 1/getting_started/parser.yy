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
%token <std::string>CLASS
%token <std::string>NEWLINE
%token <std::string>BREAK

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
%token <std::string>NOTOP

/* Functions */
%token <std::string>READ
%token <std::string>PRINT

%token END 0 "end of file"

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Operator precedence and associativity rules */
/* Used to resolve ambiguities in parsing expressions See https://www.gnu.org/software/bison/manual/bison.html#Precedence-Decl */ 
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
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
%type <Node *> entry
%type <Node *> var
%type <Node *> expression 
%type <Node *> statement
%type <Node *> type
%type <Node *> base_type

%type <Node *> program
%type <Node *> program_block

%type <Node *> list_content
%type <Node *> block_content

%type <Node *> class_list
%type <Node *> opt_class_list
%type <Node *> class_content
%type <Node *> class_block

%type <Node *> method

/* oklart */
%type <Node *> primary
%type <Node *> secondary
%type <Node *> opt_for_init
%type <Node *> opt_for_condition


%type <Node *> params
%type <Node *> opt_params
%type <Node *> param
%type <Node *> opt_args
%type <Node *> args

/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Grammar rules section */
/* This section defines the production rules for the language being parsed */
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
%%
root:
    program END   { root = $1; }
  ;


program:
    opt_class_list opt_newlines entry
  {
    Node* prog = new Node("Program","",yylineno);
    if ($1) prog->children.push_back($1);
    prog->children.push_back($3);
    $$ = prog;
  }
  | opt_class_list opt_newlines
  {
    Node* prog = new Node("Program","",yylineno);
    if ($1) prog->children.push_back($1);
    $$ = prog;
  }
  ;

opt_newlines:
  | opt_newlines NEWLINE
  ;


/*------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Classes
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

opt_class_list:
    { $$ = nullptr; }
  | class_list  { $$ = $1; }
  ;

class_list:
    CLASS ID class_block 
      {
        Node* list = new Node("ClassList","",yylineno);
        Node* cls  = new Node("Class",$2,yylineno);
        cls->children.push_back($3);
        list->children.push_back(cls);
        $$ = list;
      }
  | class_list CLASS ID class_block 
      {
        $$ = $1;
        Node* cls = new Node("Class",$3,yylineno);
        cls->children.push_back($4);
        $$->children.push_back(cls);
      }
  ;

class_block:
    LBRACE class_content RBRACE
      {
        $$ = $2;
      }
  ;

class_content:
      {
        $$ = new Node("ClassContent","", yylineno);
      }
  | class_content var 
      {
        $$ = $1;
        $$->children.push_back($2);
      }
  | class_content method 
      {
        $$ = $1;
        $$->children.push_back($2);
      }
  | class_content NEWLINE
      {
        $$ = $1;
      }
  ;


/*------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Entry point and program structure
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

entry:
    MAIN LP RP COLON TYPE_INT program_block
      {
        Node* mainNode = new Node("MainEntry","",yylineno);
        mainNode->children.push_back(new Node("Type", $5, yylineno));
        mainNode->children.push_back($6);
        $$ = mainNode;
      }
  ;

program_block:
    LBRACE block_content RBRACE
      {
        $$ = $2;
      }
  ;

block_content:
    statement                      { $$ = new Node("BlockContent","", yylineno); $$->children.push_back($1); }
  | block_content statement        { $$ = $1; $$->children.push_back($2); }
  | NEWLINE                        { $$ = new Node("BlockContent","", yylineno); }
  | block_content NEWLINE          { $$ = $1; }
  ;


/*------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Types
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/


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
/* Methods
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

method: 
    ID LP opt_params RP COLON type program_block
    {
        Node* methodNode = new Node("Method", $1, yylineno);
        if ($3) methodNode->children.push_back($3);
        methodNode->children.push_back($6);
        methodNode->children.push_back($7);
        $$ = methodNode;
      }
  ;

opt_params:
    { $$ = nullptr; }
  | params { $$ = $1; }
  ;

params:
    param
      {
        $$ = new Node("ParameterList","",yylineno);
        $$->children.push_back($1);
      }
  | params COMMA param
      {
        $$ = $1;
        $$->children.push_back($3);
      }
  ;

param:
    ID COLON type
      {
        Node* p = new Node("Param","",yylineno);
        p->children.push_back(new Node("Id",$1,yylineno));
        p->children.push_back($3);
        $$ = p;
      }
  ;

opt_args:
      { $$ = nullptr; }
  | args
      { $$ = $1; }
  ;

args:
    expression
      {
        $$ = new Node("ArgList","",yylineno);
        $$->children.push_back($1);
      }
  | args COMMA expression
      {
        $$ = $1;
        $$->children.push_back($3);
      }
  ;


/*------------------------------------------------------------------------------------------------------------------------------------------------- */
/* expression
/*-------------------------------------------------------------------------------------------------------------------------------------------------*/

primary:
    INT
      {
        $$ = new Node("Int", $1, yylineno);
      }
  | FLOAT
      {
        $$ = new Node("Float", $1, yylineno);
      }
  | ID
      {
        $$ = new Node("Id", $1, yylineno);
      }
  | LP expression RP
      {
        $$ = $2;
      }
  | base_type LBRACK list_content RBRACK
      {
        $$ = new Node("ListExpression","",yylineno);
        $$->children.push_back($1);
        $$->children.push_back($3);
      }
  ;  

secondary:
    primary
      {
        $$ = $1;
      }
  | secondary LBRACK expression RBRACK
      {
        Node* idx = new Node("IndexExpression","",yylineno);
        idx->children.push_back($1);
        idx->children.push_back($3);  
        $$ = idx;
      }
  | secondary DOT LENGTH
      {
        Node* len = new Node("LengthExpression","",yylineno);
        len->children.push_back($1);
        $$ = len;
      }
  | secondary DOT ID
      {
        Node* field = new Node("FieldAccess",$3,yylineno);
        field->children.push_back($1);
        $$ = field;
      }
  | secondary LP opt_args RP
      {
        Node* call = new Node("MethodCall","",yylineno);
        call->children.push_back($1);  
        if ($3) call->children.push_back($3);
        $$ = call;
      }
  | NOTOP primary
      {
        Node* notExpr = new Node("NotExpression","",yylineno);
        notExpr->children.push_back($2);
        $$ = notExpr;
      }
  ;


expression:
    expression PLUSOP expression
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
  | expression LEQOP expression
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
  | secondary
      {
        $$ = $1;
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
  | var ASSIGNOP expression
  {
    Node* a = new Node("AssignStatement", "", yylineno);
    a->children.push_back($1);
    a->children.push_back($3);
    $$ = a;
  }
 ;

list_content
  : expression
    {
      $$ = new Node("ListContent","", yylineno);
      $$->children.push_back($1);
    }
  | list_content COMMA expression
    {
      $$ = $1;
      $$->children.push_back($3);
    }
  ;


opt_for_init:
    { $$ = nullptr; }
  | var 
      { $$ = $1; }
  | expression ASSIGNOP expression
      {
        Node* a = new Node("AssignStatement", "", yylineno);
        a->children.push_back($1);
        a->children.push_back($3);
        $$ = a;
      }
  ;

opt_for_condition:
    { $$ = nullptr; }
  | expression 
      { $$ = $1; }
  ;


/* ------------------------------------------------------------------------------------------------------------------------------------------------- */
/* Statements
/* ------------------------------------------------------------------------------------------------------------------------------------------------- */

stmt_end:
    NEWLINE
  ;

statement:
        program_block
            {
              $$ = $1;
            }
        |var stmt_end
            {
              $$ = new Node("VarDeclaration", "", yylineno);
              $$->children.push_back($1);
            }
        | expression ASSIGNOP expression stmt_end
            {
              Node* a = new Node("AssignStatement", "", yylineno);
              a->children.push_back($1);
              a->children.push_back($3);
              $$ = a;
            }
        | PRINT LP expression RP stmt_end
            {
              $$ = new Node("PrintStatement", "", yylineno);
              $$->children.push_back($3);
            }
        | READ LP expression RP stmt_end
            {
              $$ = new Node("ReadStatement", "", yylineno);
              $$->children.push_back($3);
            }
        | BREAK stmt_end
            {
              $$ = new Node("BreakStatement", "", yylineno);
            }
        | RETURN expression stmt_end
            {
              $$ = new Node("ReturnStatement", "", yylineno);
              $$->children.push_back($2);
            }
        | FOR LP opt_for_init COMMA opt_for_condition COMMA expression ASSIGNOP expression RP statement
            {
              $$ = new Node("ForStatement", "", yylineno);
              
              Node* start_condition = new Node("start_condition", "", yylineno);
              if ($3) start_condition->children.push_back($3);
              $$->children.push_back(start_condition);

              Node* end_condition = new Node("end_condition", "", yylineno);
              if ($5) end_condition->children.push_back($5);
              $$->children.push_back(end_condition);

              Node* step = new Node("step", "", yylineno);
              step->children.push_back($7);
              step->children.push_back($9);
              $$->children.push_back(step);

              Node* block = new Node("block", "", yylineno);
              block->children.push_back($11);
              $$->children.push_back(block);
            }
        | IF LP expression RP statement %prec LOWER_THAN_ELSE
            {
              $$ = new Node("IfStatement", "", yylineno);

              Node* condition = new Node("condition", "", yylineno);
              condition->children.push_back($3);
              $$->children.push_back(condition);

              Node* then_branch = new Node("then_branch", "", yylineno);
              then_branch->children.push_back($5);
              $$->children.push_back(then_branch);
            }
        | IF LP expression RP statement ELSE statement
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
        | expression stmt_end 
            {
              $$ = new Node("ExpressionStatement", "", yylineno);
              $$->children.push_back($1);
            }
        ;
%%