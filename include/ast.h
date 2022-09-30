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
    std::string btype;
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
    std::unique_ptr<BaseAST> next;  // for optional defs

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// ConstInitVal  ::= ConstExp
// InitVal       ::= Exp
// TODO: This will be expanded for array
class InitValAST : public BaseAST {
   public:
    bool is_const;
    std::unique_ptr<BaseAST> exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class FuncDefAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class FuncTypeAST : public BaseAST {
    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class BlockAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> stmt;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class StmtAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// Exp           ::= LOrExp
// ConstExp      ::= Exp
class ExpAST : public BaseAST {
   public:
    bool is_const;
    std::unique_ptr<BaseAST> binary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
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
class BinaryExpAST : public BaseAST {
   public:
    binary_exp_ast_type type;
    std::string op;
    std::unique_ptr<BaseAST> l_exp;
    std::unique_ptr<BaseAST> r_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    UNARY_EXP_AST_TYPE_0 = 0,  // primary_exp
    UNARY_EXP_AST_TYPE_1,      // unary_op unary_exp
} unary_exp_ast_type;

// UnaryExp      ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST {
   public:
    unary_exp_ast_type type;
    std::string op;
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    PRIMARY_EXP_AST_TYPE_0 = 0,  // exp
    PRIMARY_EXP_AST_TYPE_1,      // number
} primary_exp_ast_type;

class PrimaryExpAST : public BaseAST {
   public:
    primary_exp_ast_type type;
    std::unique_ptr<BaseAST> exp;
    int number;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};