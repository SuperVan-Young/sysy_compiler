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
%token INT VOID RETURN CONST IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST
%token <str_val> LE GE EQ NE LAND LOR

// Non-terminating tokens
// If a token appears 0 or 1 time, we write two rules respectivelly.
// If a token appears 0 or multiple times, we call it Optional.
%type <ast_val> CompUnit
                FuncDef FuncFParams OptionalFuncFParam FuncFParam
                FuncRParams OptionalFuncRParam
                Decl ConstDecl ConstDef OptionalConstDef ConstInitVal 
                VarDecl VarDef OptionalVarDef InitVal
                Block OptionalBlockItem BlockItem
                Stmt MatchedStmt OpenStmt
                LVal
                ConstExp Exp LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp PrimaryExp
%type <str_val> FuncType
                BType
                UnaryOp MulOp AddOp RelOp EqOp
%type <int_val> Number

%%

Start
  : CompUnit {
    auto start = make_unique<StartAST>();
    CompUnitAST *cur = (CompUnitAST*)$1;
    CompUnitAST *tmp;
    while (cur != nullptr) {
      tmp = cur->next;
      start->units.push_back(unique_ptr<BaseAST>(cur));
      cur->next = nullptr;
      cur = tmp;
    }
    ast = move(start);
  }
  ;

CompUnit
  : FuncDef {
    auto ast = new CompUnitAST();
    ast->type = COMP_UNIT_AST_TYPE_FUNC;
    ast->func_def = unique_ptr<BaseAST>($1);
    ast->next = nullptr;
    $$ = ast;
  }
  | Decl {
    auto ast = new CompUnitAST();
    ast->type = COMP_UNIT_AST_TYPE_DECL;
    ast->decl = unique_ptr<BaseAST>($1);
    ast->next = nullptr;
    $$ = ast;
  }
  | CompUnit FuncDef {
    auto ast = new CompUnitAST();
    ast->type = COMP_UNIT_AST_TYPE_FUNC;
    ast->func_def = unique_ptr<BaseAST>($2);
    ast->next = (CompUnitAST*)$1;
    $$ = ast;
  }
  | CompUnit Decl {
    auto ast = new CompUnitAST();
    ast->type = COMP_UNIT_AST_TYPE_DECL;
    ast->decl = unique_ptr<BaseAST>($2);
    ast->next = (CompUnitAST*)$1;
    $$ = ast;
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
  | FuncType IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($6);
    FuncFParamAST* cur = (FuncFParamAST*)$4;
    FuncFParamAST* tmp;
    while (cur != nullptr) {
      tmp = cur->next;
      ast->params.push_back(unique_ptr<BaseAST>((BaseAST*)cur));
      cur->next = nullptr;
      cur = tmp;
    }
    $$ = ast;
  }
  ;

FuncType
  : VOID {
    $$ = new std::string("void");
  }
  | INT {
    $$ = new std::string("int");
  }
  ;

FuncFParams
  : FuncFParam OptionalFuncFParam {
    ((FuncFParamAST*)$1)->next = ((FuncFParamAST*)$2);
    $$ = $1;
  }
  ;

FuncFParam
  : BType IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

OptionalFuncFParam
  : ',' FuncFParam OptionalFuncFParam {
    ((FuncFParamAST*)$2)->next = ((FuncFParamAST*)$3);
    $$ = $2;
  }
  | {
    $$ = nullptr;
  }

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
  : MatchedStmt {
    $$ = $1;
  }
  | OpenStmt {
    $$ = $1;
  }
  ;

MatchedStmt
  : IF '(' Exp ')' MatchedStmt ELSE MatchedStmt {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_IF;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_ASSIGN;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_EXP;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_EXP;
    ast->exp = unique_ptr<BaseAST>(nullptr);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_BLOCK;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_RETURN;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_RETURN;
    ast->exp = unique_ptr<BaseAST>(nullptr);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_WHILE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->do_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_CONTINUE;
    $$ = ast;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_IF;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>(nullptr);
    $$ = ast;
  }
  | IF '(' Exp ')' MatchedStmt ELSE OpenStmt {
    auto ast = new StmtAST();
    ast->type = STMT_AST_TYPE_IF;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->then_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }

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
    $$ = $1;
  }
  | LOrExp LOR LAndExp {
    auto ast = new BinaryExpAST();
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    $$ = $1;
  }
  | LAndExp LAND EqExp {
    auto ast = new BinaryExpAST();
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    $$ = $1;
  }
  | EqExp EqOp RelExp {
    auto ast = new BinaryExpAST();
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    $$ = $1;
  }
  | RelExp RelOp AddExp {
    auto ast = new BinaryExpAST();
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    $$ = $1;
  }
  | AddExp AddOp MulExp {
    auto ast = new BinaryExpAST();
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    $$ = $1;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new BinaryExpAST();
    ast->l_exp = unique_ptr<BaseAST>($1);
    ast->op = std::string(*$2);
    ast->r_exp = unique_ptr<BaseAST>($3);
    delete $2;
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    $$ = $1;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = UNARY_EXP_AST_TYPE_OP;
    ast->op = string(*$1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    delete $1;
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->type = UNARY_EXP_AST_TYPE_FUNC;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->type = UNARY_EXP_AST_TYPE_FUNC;
    ast->ident = *unique_ptr<string>($1);
    FuncRParamAST* cur = (FuncRParamAST*)($3);
    FuncRParamAST* tmp;
    while (cur != nullptr) {
      tmp = cur->next;
      ast->params.push_back(move(((FuncRParamAST*)cur)->exp));
      delete cur;
      cur = tmp;
    }
    $$ = ast;
  }
  ;

FuncRParams
  : Exp OptionalFuncRParam {
    auto ast = new FuncRParamAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->next = (FuncRParamAST*)$2;
    $$ = ast;
  }
  ;

OptionalFuncRParam
  : ',' Exp OptionalFuncRParam {
    auto ast = new FuncRParamAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->next = (FuncRParamAST*)$3;
    $$ = ast;
  }
  | {
    $$ = nullptr;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    $$ = $2;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type = PRIMARY_EXP_AST_TYPE_NUMBER;
    ast->number = $1;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = PRIMARY_EXP_AST_TYPE_LVAL;
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