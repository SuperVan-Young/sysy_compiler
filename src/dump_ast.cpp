#include <ast.h>

void CompUnitAST::dump() const {
    std::cout << "CompUnitAST { ";
    func_def->dump();
    std::cout << " }";
}

void FuncDefAST::dump() const {
    std::cout << "FuncDefAST { ";
    func_type->dump();
    std::cout << ", " << ident << ", ";
    block->dump();
    std::cout << " }";
}

void FuncTypeAST::dump() const { std::cout << "FuncTypeAST { int }"; }

void BlockAST::dump() const {
    std::cout << "BlockAST { ";
    stmt->dump();
    std::cout << " }";
}

void StmtAST::dump() const {
    std::cout << "StmtAST { ";
    std::cout << number;
    std::cout << " }";
}