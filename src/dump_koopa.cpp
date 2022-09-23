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
    add_exp->dump_koopa(irgen, out);
    return; // needless to operate on stack
}

void AddExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == ADD_EXP_AST_TYPE_0) {
        mul_exp->dump_koopa(irgen, out);
        return;  // needless to operate on stack
    } else if (type == ADD_EXP_AST_TYPE_1) {
        add_exp->dump_koopa(irgen, out);
        mul_exp->dump_koopa(irgen, out);

        // fetch sub exp values
        auto mul_val = irgen.stack_val.top();
        irgen.stack_val.pop();
        auto add_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        // dump exp w.r.t. op
        std::string exp_val;
        exp_val = irgen.new_val();
        out << "  " << exp_val << " = ";
        if (op == "+")
            out << "add ";
        else if (op == "-")
            out << "sub ";
        else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);
        }
        out << add_val << ", " <<  mul_val << std::endl;
        
        irgen.stack_val.push(exp_val);
    } else {
        assert(false);
    }
}

void MulExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == MUL_EXP_AST_TYPE_0) {
        unary_exp->dump_koopa(irgen, out);
        return;  // needless to operate on stack_val
    } else if (type == MUL_EXP_AST_TYPE_1) {
        mul_exp->dump_koopa(irgen, out);
        unary_exp->dump_koopa(irgen, out);

        // fetch sub exp values
        auto unary_val = irgen.stack_val.top();
        irgen.stack_val.pop();
        auto mul_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        // dump exp w.r.t. op
        std::string exp_val;
        exp_val = irgen.new_val();
        out << "  " << exp_val << " = ";
        if (op == "*") {
            out << "mul ";
        } else if (op == "/") {
            out << "div ";
        } else if (op == "%") {
            out << "mod ";
        } else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);
        }
        out << mul_val << ", " <<  unary_val << std::endl;
        
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
        unary_op->dump_koopa(irgen, out);
        unary_exp->dump_koopa(irgen, out);

        auto sub_val = irgen.stack_val.top();  // fetch sub expression's token
        irgen.stack_val.pop();
        auto op = irgen.stack_val.top();  // fetch op
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
            exp_val = sub_val;
        } else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);  // Not Implemented
        }

        irgen.stack_val.push(exp_val);  // push exp token to stack
    } else {
        assert(false); // invalid type
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

void UnaryOpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    irgen.stack_val.push(op);  // save unnecessary casting
}