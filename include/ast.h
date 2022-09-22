#pragma once
#include <iostream>
#include <memory>
#include <string>

class BaseAST {
   public:
    virtual ~BaseAST() = default;

    virtual void dump() const = 0;
    virtual void dump_koopa() const = 0;
};

class CompUnitAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_def;

    void dump() const override;
    void dump_koopa() const override;
};

class FuncDefAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void dump() const override;
    void dump_koopa() const override;
};

class FuncTypeAST : public BaseAST {
    void dump() const override;
    void dump_koopa() const override;
};

class BlockAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> stmt;

    void dump() const override;
    void dump_koopa() const override;
};

class StmtAST : public BaseAST {
   public:
    int number;

    void dump() const override;
    void dump_koopa() const override;
};