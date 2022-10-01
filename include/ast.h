#pragma once
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "irgen.h"

// Base class of AST
class BaseAST {
   public:
    virtual ~BaseAST() = default;

    virtual void dump_koopa(IRGenerator &irgen, std::ostream &out) const = 0;
};

// CompUnit       ::= FuncDef
class CompUnitAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_def;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// Decl          ::= ConstDecl | VarDecl
// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";"
// VarDecl       ::= BType VarDef {"," VarDef} ";"
class DeclAST : public BaseAST {
   public:
    bool is_const;
    std::string btype;  // only int
    std::vector<std::unique_ptr<BaseAST>> defs;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// ConstDef      ::= IDENT "=" ConstInitVal
// VarDef        ::= IDENT | IDENT "=" InitVal
class DeclDefAST : public BaseAST {
   public:
    bool is_const;
    std::string ident;
    std::unique_ptr<BaseAST> init_val;
    DeclDefAST *next;  // for optional defs

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// ConstInitVal  ::= ConstExp
// InitVal       ::= Exp
// TODO: This will be expanded for array
class InitValAST : public BaseAST {
   public:
    bool is_const;
    std::unique_ptr<BaseAST> exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override {
        assert(false);  // this function shouldn't be called
    }
};

// FuncDef       ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST {
   public:
    std::string func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// Block         ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {
   public:
    std::vector<std::unique_ptr<BaseAST>> items;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    BLOCK_ITEM_AST_TYPE_0 = 0,  // decl
    BLOCK_ITEM_AST_TYPE_1,      // stmt
} block_item_ast_type;

// BlockItem     ::= Decl | Stmt;
class BlockItemAST : public BaseAST {
   public:
    block_item_ast_type type;
    std::unique_ptr<BaseAST> item;
    BlockItemAST *next;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    STMT_AST_TYPE_0 = 0,  // assign
    STMT_AST_TYPE_1,      // return
} stmt_ast_type;

// Stmt          ::= LVal "=" Exp ";"
//                 | "return" Exp ";"
class StmtAST : public BaseAST {
   public:
    stmt_ast_type type;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class CalcAST : public BaseAST {
   public:
    virtual bool calc_val(IRGenerator &irgen, int &result) const = 0;
};

// ConstExp      ::= Exp
// Exp           ::= LOrExp
class ExpAST : public CalcAST {
   public:
    bool is_const;
    std::unique_ptr<BaseAST> binary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result) const override;
};

typedef enum {
    BINARY_EXP_AST_TYPE_0 = 0,  // r_exp
    BINARY_EXP_AST_TYPE_1       // l_exp op r_exp
} binary_exp_ast_type;

// MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
// AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
// RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
// EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
// LAndExp       ::= EqExp | LAndExp "&&" EqExp;
// LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
class BinaryExpAST : public CalcAST {
   public:
    binary_exp_ast_type type;
    std::string op;
    std::unique_ptr<BaseAST> l_exp;
    std::unique_ptr<BaseAST> r_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result) const override;
};

typedef enum {
    UNARY_EXP_AST_TYPE_0 = 0,  // primary_exp
    UNARY_EXP_AST_TYPE_1,      // unary_op unary_exp
} unary_exp_ast_type;

// UnaryExp      ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public CalcAST {
   public:
    unary_exp_ast_type type;
    std::string op;
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result) const override;
};

typedef enum {
    PRIMARY_EXP_AST_TYPE_0 = 0,  // exp
    PRIMARY_EXP_AST_TYPE_1,      // number
    PRIMARY_EXP_AST_TYPE_2,      // lval
} primary_exp_ast_type;

class PrimaryExpAST : public CalcAST {
   public:
    primary_exp_ast_type type;
    std::unique_ptr<BaseAST> exp;
    int number;
    std::unique_ptr<BaseAST> lval;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result) const override;
};

class LValAST : public CalcAST {
   public:
    std::string ident;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result) const override;
};