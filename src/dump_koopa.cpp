#include <ast.h>

void CompUnitAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    func_def->dump_koopa(irgen, out);
}

void FuncDefAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    out << "fun @" << ident;
    out << "(): ";  // TODO: add params
    func_type->dump_koopa(irgen, out);
    out << " {" << std::endl;
    out << "\%entry:" << std::endl;
    block->dump_koopa(irgen, out);
    out << "}" << std::endl;
}

void FuncTypeAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    out << "i32";
}

void BlockAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    stmt->dump_koopa(irgen, out);
}

void StmtAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    exp->dump_koopa(irgen, out);
    auto exp = irgen.stack_val.top();
    irgen.stack_val.pop();
    out << "  ret " << exp << std::endl;
}

void ExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    binary_exp->dump_koopa(irgen, out);
    return;  // needless to operate on stack
}

void BinaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == BINARY_EXP_AST_TYPE_0) {
        r_exp->dump_koopa(irgen, out);
        return;  // needless to operate on stack
    } else if (type == BINARY_EXP_AST_TYPE_1) {
        // dump &  fetch sub exp values
        r_exp->dump_koopa(irgen, out);  // higher priority!
        l_exp->dump_koopa(irgen, out);
        auto l_val = irgen.stack_val.top();  // order!
        irgen.stack_val.pop();
        auto r_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        // dump exp w.r.t. op
        if (op == "&&" || op == "||") {
            // convert both value into logical values
            auto ll_val = irgen.new_val();
            auto lr_val = irgen.new_val();
            out << "  " << ll_val << " = ne " << l_val << ", 0" << std::endl;
            out << "  " << lr_val << " = ne " << r_val << ", 0" << std::endl;
            l_val = ll_val;
            r_val = lr_val;
        }
        auto exp_val = irgen.new_val();
        out << "  " << exp_val << " = ";
        if (op == "+")
            out << "add ";
        else if (op == "-") {
            out << "sub ";
        } else if (op == "*") {
            out << "mul ";
        } else if (op == "/") {
            out << "div ";
        } else if (op == "%") {
            out << "mod ";
        } else if (op == "<") {
            out << "lt ";
        } else if (op == ">") {
            out << "gt ";
        } else if (op == "<=") {
            out << "le ";
        } else if (op == ">=") {
            out << "ge ";
        } else if (op == "==") {
            out << "eq ";
        } else if (op == "!=") {
            out << "ne ";
        } else if (op == "&&") {
            out << "and ";
        } else if (op == "||") {
            out << "or ";
        } else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);
        }
        out << l_val << ", " << r_val << std::endl;
        irgen.stack_val.push(exp_val);

    } else {
        assert(false);
    }
}

void UnaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == UNARY_EXP_AST_TYPE_0) {
        // primary_exp
        primary_exp->dump_koopa(irgen, out);
        return;  // needless to operate on stack
    } else if (type == UNARY_EXP_AST_TYPE_1) {
        // unary_op unary_exp
        unary_exp->dump_koopa(irgen, out);

        auto sub_val = irgen.stack_val.top();  // fetch sub expression's token
        irgen.stack_val.pop();

        // dump exp w.r.t op
        std::string exp_val;
        if (op == "!") {
            exp_val = irgen.new_val();
            out << "  " << exp_val << " = ";
            out << "eq " << sub_val << ", 0" << std::endl;
        } else if (op == "-") {
            exp_val = irgen.new_val();
            out << "  " << exp_val << " = ";
            out << "sub 0, " << sub_val << std::endl;
        } else if (op == "+") {
            exp_val = sub_val;  // ignore
        } else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);  // Not Implemented
        }

        irgen.stack_val.push(exp_val);  // push exp token to stack
    } else {
        assert(false);  // invalid type
    }
}

void PrimaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == PRIMARY_EXP_AST_TYPE_0) {
        // exp
        exp->dump_koopa(irgen, out);
        return;  // needless to operate on stack
    } else if (type == PRIMARY_EXP_AST_TYPE_1) {
        // number
        irgen.stack_val.push(std::to_string(number));
        return;
    } else {
        assert(false);
    }
}