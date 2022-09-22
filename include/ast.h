#pragma once
#include <iostream>
#include <memory>
#include <string>

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
    int number;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};