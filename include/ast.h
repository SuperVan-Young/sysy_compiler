#pragma once
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

#include "irgen.h"

class BaseAST {
   public:
    virtual ~BaseAST() = default;

    virtual void dump_koopa(IRGenerator &irgen, std::ostream &out) const = 0;
};

class CompUnitAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_def;

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

class ExpAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> binary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    BINARY_EXP_AST_TYPE_0 = 0,  // r_exp
    BINARY_EXP_AST_TYPE_1       // l_exp op r_exp
} binary_exp_ast_type;

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