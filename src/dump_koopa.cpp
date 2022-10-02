#include <ast.h>

// check if exp's operand is named or temp symbol
bool is_symbol(std::string operand) {
    auto c = operand[0];
    return c == '%' || c == '@';
}

void CompUnitAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    func_def->dump_koopa(irgen, out);
}

void DeclAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    for (auto &def : defs) def->dump_koopa(irgen, out);
}

void DeclDefAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    assert(!irgen.symbol_table.exist_entry(ident));
    SymbolTableEntry entry;

    if (is_const) {
        entry.is_const = true;
        // add const entry into symbol table
        auto exp = dynamic_cast<InitValAST *>(init_val.get())->exp.get();
        dynamic_cast<CalcAST *>(exp)->calc_val(irgen, entry.val, true);
        irgen.symbol_table.insert_entry(ident, entry);
    } else {
        entry.is_const = false;
        // allocate memory for var
        out << "  @" << ident << " = alloc i32" << std::endl;
        // add var entry into symbol table
        if (init_val.get()) {
            // store initial value to memory, if there is
            auto exp = dynamic_cast<InitValAST *>(init_val.get())->exp.get();
            exp->dump_koopa(irgen, out);
            auto store_val = irgen.stack_val.top();
            irgen.stack_val.pop();
            out << "  store " << store_val << ", @" << ident << std::endl;
        }
        irgen.symbol_table.insert_entry(ident, entry);
    }
}

void FuncDefAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    out << "fun @" << ident << "(): "
        << "i32"
        << " {" << std::endl;

    // prepare the first block for control flow
    auto block_info = BasicBlockInfo();
    auto block_name = irgen.new_block();
    irgen.control_flow.insert_info(block_name, block_info);
    out << block_name << ":" << std::endl;

    block->dump_koopa(irgen, out);

    // check if basic block has returned, and give up control flow
    if (irgen.control_flow.check_ending_status() !=
        BASIC_BLOCK_ENDING_STATUS_RETURN) {
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_RETURN);
        out << "  ret 114514" << std::endl;  // return random const
    }
    irgen.control_flow.cur_block = "";
    irgen.control_flow.to_next_block = false;

    out << "}" << std::endl;
}

void BlockAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    irgen.symbol_table.push_block();
    if (irgen.control_flow.to_next_block) {
        irgen.control_flow.switch_control();
        out << irgen.control_flow.cur_block << ":" << std::endl;
    }

    for (auto &item : items) {
        item->dump_koopa(irgen, out);
        // needless to care about if-else / while here
        // only take care of early return, raised by return stmt
        if (irgen.control_flow.check_ending_status() ==
            BASIC_BLOCK_ENDING_STATUS_RETURN) {
            break;
        }
    }

    irgen.symbol_table.pop_block();
    // block doesn't change control flow on itself
    // if-else / while is handled in Stmt
    // Fix-up return is handled in func
}

void BlockItemAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    item->dump_koopa(irgen, out);
}

void StmtAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == STMT_AST_TYPE_0) {
        // assign
        exp->dump_koopa(irgen, out);
        auto r_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        // lval shouldn't be const
        auto lval_name = dynamic_cast<LValAST *>(lval.get())->ident;
        assert(!irgen.symbol_table.is_const_entry(lval_name));
        out << "  store " << r_val << ", @" << lval_name << std::endl;
    } else if (type == STMT_AST_TYPE_1) {
        // return
        if (exp.get() != nullptr) {
            exp->dump_koopa(irgen, out);
            auto ret_val = irgen.stack_val.top();
            irgen.stack_val.pop();
            out << "  ret " << ret_val << std::endl;
        } else {
            out << "  ret 114514" << std::endl;  // return random const
        }
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_RETURN);  // block should return
    } else if (type == STMT_AST_TYPE_2) {
        // exp
        if (exp.get() != nullptr) {
            exp->dump_koopa(irgen, out);
            irgen.stack_val.pop();  // no one use it
        }
    } else if (type == STMT_AST_TYPE_3) {
        // block
        block->dump_koopa(irgen, out);
    } else {
        assert(false);
    }
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

        // Optimization: calculate directly if l_val and r_val are const
        if (!is_symbol(l_val) && !is_symbol(r_val)) {
            int lhs = std::stoi(l_val);
            int rhs = std::stoi(r_val);
            int ret;
            if (op == "+")
                ret = lhs + rhs;
            else if (op == "-")
                ret = lhs - rhs;
            else if (op == "*")
                ret = lhs * rhs;
            else if (op == "/")
                ret = lhs / rhs;
            else if (op == "%")
                ret = lhs % rhs;
            else if (op == "<")
                ret = lhs < rhs;
            else if (op == ">")
                ret = lhs > rhs;
            else if (op == "<=")
                ret = lhs <= rhs;
            else if (op == ">=")
                ret = lhs >= rhs;
            else if (op == "==")
                ret = lhs == rhs;
            else if (op == "!=")
                ret = lhs != rhs;
            else if (op == "&&")
                ret = lhs && rhs;
            else if (op == "||")
                ret = lhs || rhs;
            else {
                std::cerr << "Invalid op: " << op << std::endl;
                assert(false);
            }
            irgen.stack_val.push(std::to_string(ret));
            return;
        }

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

        // Optimization: calculate directly if sub_val is const
        if (!is_symbol(sub_val)) {
            int val = std::stoi(sub_val);
            int ret;
            if (op == "!")
                ret = !val;
            else if (op == "-")
                ret = -val;
            else if (op == "+")
                ret = val;
            else {
                std::cerr << "Invalid op: " << op << std::endl;
                assert(false);
            }
            irgen.stack_val.push(std::to_string(ret));
            return;
        }

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
    } else if (type == PRIMARY_EXP_AST_TYPE_2) {
        // lval
        lval->dump_koopa(irgen, out);
        return;  // needless to operate on stack
    } else {
        std::cerr << "Invalid primary exp type: " << type << std::endl;
        assert(false);
    }
}

void LValAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    assert(irgen.symbol_table.exist_entry(ident));

    if (irgen.symbol_table.is_const_entry(ident)) {
        int val;
        irgen.symbol_table.get_entry_val(ident, val);
        irgen.stack_val.push(std::to_string(val));
    } else {
        auto val = irgen.new_val();
        out << "  " << val << " = load "
            << "@" << ident << std::endl;
        irgen.stack_val.push(val);
    }
}