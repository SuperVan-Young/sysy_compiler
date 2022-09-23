%code requires {
  #include <memory>
  #include <string>
  #include <ast.h>
}

%{
/**********************************************************************
 * PARSER
 **********************************************************************/

// The "Code requires" part goes to header files generated by bison.
// sysy.l needs this header file.

// Global settings, which will be added to bison source code

#include <iostream>
#include <memory>
#include <string>
#include <ast.h>

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);
extern int yylineno;

using namespace std;

%}

// parser func yyparse's & yyerror's arguments
%parse-param { std::unique_ptr<BaseAST> &ast }

// definition of yylval as union, where lexer returns token's attribute value
// NOTICE that we don't use unique ptr in union, since it causes bugs.
// The parent node in AST should be responsible in making unique ptr to children.
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// manifest constant for lexer, representing terminating token
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

// Non-terminating tokens
%type <ast_val> FuncDef FuncType Block Stmt Exp
%type <ast_val> LOrExp LAndExp EqExp RelExp AddExp MulExp PrimaryExp UnaryExp
%type <str_val> UnaryOp BinaryOp
%type <int_val> Number

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast->stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->binary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_0;
    ast->r_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp BinaryOp LAndExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_0;
    ast->r_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp BinaryOp EqExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_0;
    ast->r_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp BinaryOp RelExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_0;
    ast->r_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp BinaryOp RelExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_0;
    ast->r_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp BinaryOp MulExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_0;
    ast->r_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp BinaryOp UnaryExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->type = PRIMARY_EXP_AST_TYPE_0;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type = PRIMARY_EXP_AST_TYPE_1;
    ast->number = $1;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->type = UNARY_EXP_AST_TYPE_0;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = UNARY_EXP_AST_TYPE_1;
    ast->op = string(*$1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    delete $1;
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$ = new std::string("+");
  }
  | '-' {
    $$ = new std::string("-");
  }
  | '!' {
    $$ = new std::string("!");
  }
  ;

BinaryOp
  : '+'  { $$ = new std::string("+"); }
  | '-'  { $$ = new std::string("-"); }
  | '*'  { $$ = new std::string("*"); }
  | '/'  { $$ = new std::string("/"); }
  | '%'  { $$ = new std::string("%"); }
  | '<'  { $$ = new std::string("<"); }
  | '>'  { $$ = new std::string(">"); }
  | '<' '='  { $$ = new std::string("<="); }
  | '>' '='  { $$ = new std::string(">="); }
  | '=' '='  { $$ = new std::string("=="); }
  | '!' '='  { $$ = new std::string("!="); }
  | '&' '&'  { $$ = new std::string("&&"); }
  | '|' '|'  { $$ = new std::string("||"); }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

%%

void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "line " << yylineno << ": " << s << endl;
}