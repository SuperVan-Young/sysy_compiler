#pragma once
#include <iostream>
#include <memory>
#include <string>

class BaseAST {
   public:
    virtual ~BaseAST() = default;

    virtual void dump() const = 0;
};

class CompUnitAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_def;

    void dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->dump();
        std::cout << " }";
    }
};

class FuncDefAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->dump();
        std::cout << ", " << ident << ", ";
        block->dump();
        std::cout << " }";
    }
};

class FuncTypeAST : public BaseAST {
    void dump() const override { std::cout << "FuncTypeAST { int }"; }
};

class BlockAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> stmt;

    void dump() const override {
        std::cout << "BlockAST { ";
        stmt->dump();
        std::cout << " }";
    }
};

class StmtAST : public BaseAST {
   public:
    int number;

    void dump() const override {
        std::cout << "StmtAST { ";
        std::cout << number;
        std::cout << " }";
    }
};