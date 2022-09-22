#include <ast.h>

void CompUnitAST::dump_koopa(std::ostream &out) const {
    func_def->dump_koopa(out);
}

void FuncDefAST::dump_koopa(std::ostream &out) const {
    out << "fun @" << ident;
    out << "(): ";  // TODO: add params
    func_type->dump_koopa(out);
    out << " {" << std::endl;
    out << "\%entry:" << std::endl;
    block->dump_koopa(out);
    out << "}" << std::endl;
}

void FuncTypeAST::dump_koopa(std::ostream &out) const {
    out << "i32";
}

void BlockAST::dump_koopa(std::ostream &out) const {
    stmt->dump_koopa(out);
}

void StmtAST::dump_koopa(std::ostream &out) const {
    out << "  ";
    out << "ret " << number;
    out << std::endl;
}
