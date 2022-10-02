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
%token INT RETURN CONST
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> LE GE EQ NE LAND LOR

// Non-terminating tokens
%type <ast_val> FuncDef 
                Decl ConstDecl ConstDef OptionalConstDef ConstInitVal 
                VarDecl VarDef OptionalVarDef InitVal
                Block OptionalBlockItem BlockItem Stmt 
                LVal
                ConstExp Exp LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp PrimaryExp
%type <str_val> FuncType
                BType
                UnaryOp MulOp AddOp RelOp EqOp
%type <int_val> Number

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;


Decl
  : ConstDecl {
    $$ = $1;
  }
  | VarDecl {
    $$ = $1;
  }
  ;

ConstDecl 
  : CONST BType ConstDef OptionalConstDef ';' {
    auto ast = new DeclAST();
    ast->is_const = true;
    ast->btype = *unique_ptr<std::string>($2);
    ast->defs.push_back(unique_ptr<BaseAST>($3));
    DeclDefAST* cur = (DeclDefAST*)$4;
    DeclDefAST* tmp;
    while (cur != nullptr) {
      tmp = cur->next;
      ast->defs.push_back(unique_ptr<BaseAST>((BaseAST*)cur));
      cur = tmp;
    }
    $$ = ast;
  }
  ;

BType
  : INT {
    $$ = new std::string("int");
  }
  ;

OptionalConstDef
  : ',' ConstDef OptionalConstDef {
    auto ast = $2;
    ((DeclDefAST*)ast)->next = ((DeclDefAST*)$3);
    $$ = ast;
  }
  | {
    $$ = nullptr;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new DeclDefAST();
    ast->is_const = true;
    ast->ident = *unique_ptr<std::string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new InitValAST();
    ast->is_const = true;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDef OptionalVarDef ';' {
    auto ast = new DeclAST();
    ast->is_const = false;
    ast->btype = *unique_ptr<std::string>($1);
    ast->defs.push_back(unique_ptr<BaseAST>($2));
    DeclDefAST* cur = (DeclDefAST*)$3;
    DeclDefAST* tmp;
    while (cur != nullptr) {
      tmp = cur->next;
      ast->defs.push_back(unique_ptr<BaseAST>((BaseAST*)cur));
      cur = tmp;
    }
    $$ = ast;
  }
  ;

OptionalVarDef
  : ',' VarDef OptionalVarDef {
    auto ast = $2;
    ((DeclDefAST*)ast)->next = ((DeclDefAST*)$3);
    $$ = ast;
  }
  | {
    $$ = nullptr;
  }
  ;

VarDef
  : IDENT {
    auto ast = new DeclDefAST();
    ast->is_const = false;
    ast->ident = *unique_ptr<std::string>($1);
    ast->init_val = unique_ptr<BaseAST>(nullptr);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new DeclDefAST();
    ast->is_const = false;
    ast->ident = *unique_ptr<std::string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->is_const = false;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    $$ = new std::string("int");
  }
  ;

Block
  : '{' OptionalBlockItem '}' {
    auto ast = new BlockAST();
    BlockItemAST* cur = (BlockItemAST*)($2);
    BlockItemAST* tmp;
    while (cur != nullptr) {
      tmp = cur->next;
      ast->items.push_back(unique_ptr<BaseAST>((BaseAST*)cur));
      cur = tmp;
    }
    $$ = ast;
  }
  ;

OptionalBlockItem
  : BlockItem OptionalBlockItem {
    auto ast = $1;
    ((BlockItemAST*)ast)->next = ((BlockItemAST*)$2);
    $$ = ast;
  }
  | {
    $$ = nullptr;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->type = BLOCK_ITEM_AST_TYPE_0;
    ast->item = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->type = BLOCK_ITEM_AST_TYPE_1;
    ast->item = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<std::string>($1);
    $$ = ast;
  }

Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_0;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_2;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_2;
    ast->exp = unique_ptr<BaseAST>(nullptr);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_3;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_1;
    ast->exp = unique_ptr<BaseAST>(nullptr);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    $$ = $1;
    ((ExpAST*)$$)->is_const = true;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->is_const = false;
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
  | LOrExp LOR LAndExp {
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
  | LAndExp LAND EqExp {
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
  | EqExp EqOp RelExp {
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
  | RelExp RelOp AddExp {
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
  | AddExp AddOp MulExp {
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
  | MulExp MulOp UnaryExp {
    auto ast = new BinaryExpAST();
    ast->type = BINARY_EXP_AST_TYPE_1;
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
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
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = PRIMARY_EXP_AST_TYPE_2;
    ast->lval = unique_ptr<BaseAST>($1);
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

MulOp
  : '*'  { $$ = new std::string("*"); }
  | '/'  { $$ = new std::string("/"); }
  | '%'  { $$ = new std::string("%"); }
  ;

AddOp
  : '+'  { $$ = new std::string("+"); }
  | '-'  { $$ = new std::string("-"); }
  ;

RelOp
  : '<'  { $$ = new std::string("<"); }
  | '>'  { $$ = new std::string(">"); }
  | LE   { $$ = $1; }
  | GE   { $$ = $1; }
  ;

EqOp
  : EQ   { $$ = $1; }
  | NE   { $$ = $1; }
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