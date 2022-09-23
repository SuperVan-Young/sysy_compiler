#pragma once
#include <iostream>
#include <memory>
#include <cstring>
#include <cassert>

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
    std::unique_ptr<BaseAST> unary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    UNARY_EXP_AST_TYPE_0 = 0,
    UNARY_EXP_AST_TYPE_1,
} unary_exp_ast_type;

class UnaryExpAST : public BaseAST {
   public:
    unary_exp_ast_type type;
    // case 0
    std::unique_ptr<BaseAST> primary_exp;
    // case 1
    std::unique_ptr<BaseAST> unary_op;
    std::unique_ptr<BaseAST> unary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    PRIMARY_EXP_AST_TYPE_0 = 0,
    PRIMARY_EXP_AST_TYPE_1,
} primary_exp_ast_type;

class PrimaryExpAST : public BaseAST {
   public:
    primary_exp_ast_type type;
    // case 0
    std::unique_ptr<BaseAST> exp;
    // case 1
    int number;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class UnaryOpAST : public BaseAST {
   public:
    std::string op;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};
