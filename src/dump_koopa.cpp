#include <ast.h>

void CompUnitAST::dump_koopa() const {
    func_def->dump_koopa();
}

void FuncDefAST::dump_koopa() const {
    std::cout << "fun @" << ident;
    std::cout << "(): ";  // TODO: add params
    func_type->dump_koopa();
    std::cout << " {" << std::endl;
    std::cout << "\%entry:" << std::endl;
    block->dump_koopa();
    std::cout << "}" << std::endl;
}

void FuncTypeAST::dump_koopa() const {
    std::cout << "i32";
}

void BlockAST::dump_koopa() const {
    stmt->dump_koopa();
}

void StmtAST::dump_koopa() const {
    std::cout << "  ";
    std::cout << "ret " << number;
    std::cout << std::endl;
}
