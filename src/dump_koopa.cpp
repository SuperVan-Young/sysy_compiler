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
    // insert will automatically check duplicated entry
    SymbolTableEntry entry;

    if (is_const) {
        entry.is_const = true;
        // add const entry into symbol table
        auto exp = dynamic_cast<InitValAST *>(init_val.get())->exp.get();
        dynamic_cast<CalcAST *>(exp)->calc_val(irgen, entry.val, true);
        irgen.symbol_table.insert_entry(ident, entry);
    } else {
        entry.is_const = false;

        // store initial value to memory, if there is
        std::string store_val;
        if (init_val.get()) {
            auto exp = dynamic_cast<InitValAST *>(init_val.get())->exp.get();
            exp->dump_koopa(irgen, out);
            store_val = irgen.stack_val.top();
            irgen.stack_val.pop();
        }
        irgen.symbol_table.insert_entry(ident, entry);

        auto aliased_name = irgen.symbol_table.get_aliased_name(ident);
        out << "  @" << aliased_name << " = alloc i32" << std::endl;
        if (init_val.get()) {
            out << "  store " << store_val << ", @" << aliased_name
                << std::endl;
        }
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
    irgen.control_flow.cur_block = block_name;
    out << block_name << ":" << std::endl;

    block->dump_koopa(irgen, out);

    // check if basic block has returned, and give up control flow
    if (irgen.control_flow.check_ending_status() ==
        BASIC_BLOCK_ENDING_STATUS_NULL) {
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_RETURN);
        out << "  ret 1919810" << std::endl;  // return random const
    }
    irgen.control_flow.cur_block = "";

    out << "}" << std::endl;
}

void BlockAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    irgen.symbol_table.push_block();

    for (auto &item : items) {
        // if block has ending status, early exit.
        auto status = irgen.control_flow.check_ending_status();
        if (status != BASIC_BLOCK_ENDING_STATUS_NULL)
            break;
        item->dump_koopa(irgen, out);
    }

    irgen.symbol_table.pop_block();
}

void BlockItemAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    item->dump_koopa(irgen, out);
}

// dump stmt and take care of control flow
static void dump_koopa_if_stmt(BaseAST *stmt, std::string cur_block,
                               std::string end_block, IRGenerator &irgen,
                               std::ostream &out) {
    irgen.control_flow.cur_block = cur_block;
    out << cur_block << ":" << std::endl;
    stmt->dump_koopa(irgen, out);
    // jump to ending when current block hasn't finished
    if (irgen.control_flow.check_ending_status() ==
        BASIC_BLOCK_ENDING_STATUS_NULL) {
        out << "  jump " << end_block << std::endl;
        irgen.control_flow.modify_ending_status(BASIC_BLOCK_ENDING_STATUS_JUMP);
    }
}

void StmtAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == STMT_AST_TYPE_ASSIGN) {
        assert(exp.get() != nullptr);
        exp->dump_koopa(irgen, out);
        auto r_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        // lval shouldn't be const
        auto lval_name = dynamic_cast<LValAST *>(lval.get())->ident;
        assert(!irgen.symbol_table.is_const_entry(lval_name));
        auto aliased_lval_name = irgen.symbol_table.get_aliased_name(lval_name);
        out << "  store " << r_val << ", @" << aliased_lval_name << std::endl;
    } else if (type == STMT_AST_TYPE_RETURN) {
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
    } else if (type == STMT_AST_TYPE_EXP) {
        if (exp.get() != nullptr) {
            exp->dump_koopa(irgen, out);
            irgen.stack_val.pop();  // no one use it
        }
    } else if (type == STMT_AST_TYPE_BLOCK) {
        block->dump_koopa(irgen, out);
    } else if (type == STMT_AST_TYPE_IF) {
        // dump condition expression
        assert(exp.get() != nullptr);
        exp->dump_koopa(irgen, out);
        auto cond = irgen.stack_val.top();
        irgen.stack_val.pop();

        // generate then, else, end basic blocks
        assert(then_stmt.get() != nullptr);
        auto then_block_name = irgen.new_block();
        auto end_block_name = irgen.new_block();

        auto block_info = BasicBlockInfo();  // inherite dst break/continue
        block_info.dst_break = irgen.control_flow.get_dst_break();
        block_info.dst_continue = irgen.control_flow.get_dst_continue();

        irgen.control_flow.insert_info(then_block_name, block_info);
        irgen.control_flow.insert_info(end_block_name, block_info);

        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_BRANCH);
        if (else_stmt.get() != nullptr) {
            auto else_block_name = irgen.new_block();
            irgen.control_flow.insert_info(else_block_name, block_info);
            out << "  br " << cond << ", " << then_block_name << ", "
                << else_block_name << std::endl;

            dump_koopa_if_stmt(then_stmt.get(), then_block_name, end_block_name,
                               irgen, out);
            dump_koopa_if_stmt(else_stmt.get(), else_block_name, end_block_name,
                               irgen, out);

            irgen.control_flow.cur_block = end_block_name;
            // Optimization: two return stmt makes end stmt unreachable
            auto then_status =
                irgen.control_flow.check_ending_status(then_block_name);
            auto else_status =
                irgen.control_flow.check_ending_status(else_block_name);
            if (then_status == BASIC_BLOCK_ENDING_STATUS_RETURN &&
                else_status == BASIC_BLOCK_ENDING_STATUS_RETURN) {
                irgen.control_flow.modify_ending_status(
                    BASIC_BLOCK_ENDING_STATUS_UNREACHABLE);
            } else {
                out << end_block_name << ":" << std::endl;
            }

        } else {
            out << "  br " << cond << ", " << then_block_name << ", "
                << end_block_name << std::endl;
            dump_koopa_if_stmt(then_stmt.get(), then_block_name, end_block_name,
                               irgen, out);
            irgen.control_flow.cur_block = end_block_name;
            out << end_block_name << ":" << std::endl;
        }
    } else if (type == STMT_AST_TYPE_WHILE) {
        assert(exp.get() != nullptr);
        assert(do_stmt.get() != nullptr);

        // generate entry, body, end block
        auto entry_block_name = irgen.new_block();
        auto body_block_name = irgen.new_block();
        auto end_block_name = irgen.new_block();

        auto body_block_info = BasicBlockInfo();  // new dsts
        body_block_info.dst_break = end_block_name;
        body_block_info.dst_continue = entry_block_name;

        auto end_block_info = BasicBlockInfo();  // old dsts
        end_block_info.dst_break = irgen.control_flow.get_dst_break();
        end_block_info.dst_continue = irgen.control_flow.get_dst_continue();

        irgen.control_flow.insert_info(entry_block_name, BasicBlockInfo());
        irgen.control_flow.insert_info(body_block_name, body_block_info);
        irgen.control_flow.insert_info(end_block_name, end_block_info);

        // finish current block
        out << "  jump " << entry_block_name << std::endl;
        irgen.control_flow.modify_ending_status(BASIC_BLOCK_ENDING_STATUS_JUMP);

        // dump entry block
        irgen.control_flow.cur_block = entry_block_name;
        out << entry_block_name << ":" << std::endl;
        exp->dump_koopa(irgen, out);
        auto cond = irgen.stack_val.top();
        irgen.stack_val.pop();
        out << "  br " << cond << ", " << body_block_name << ", "
            << end_block_name << std::endl;
        irgen.control_flow.modify_ending_status(BASIC_BLOCK_ENDING_STATUS_BRANCH);

        // dump body block, same as if-then-else stmt
        dump_koopa_if_stmt(do_stmt.get(), body_block_name, entry_block_name,
                           irgen, out);

        // dump end block:
        // TODO: maybe this block is unreachable as well
        irgen.control_flow.cur_block = end_block_name;
        out << end_block_name << ":" << std::endl;
    } else if (type == STMT_AST_TYPE_BREAK) {
        auto dst_break = irgen.control_flow.get_dst_break();
        assert(dst_break != "");
        out << "  jump " << dst_break;
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_BREAK);
    } else if (type == STMT_AST_TYPE_CONTINUE) {
        auto dst_continue = irgen.control_flow.get_dst_continue();
        assert(dst_continue != "");
        out << "  jump " << dst_continue;
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_CONTINUE);
    } else {
        assert(false);
    }
}

void ExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    binary_exp->dump_koopa(irgen, out);
    return;  // needless to operate on stack
}

void BinaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    l_exp->dump_koopa(irgen, out);
    auto l_val = irgen.stack_val.top();
    irgen.stack_val.pop();

    // Optimization: short circult r_exp
    if (!is_symbol(l_val)) {
        auto lhs = std::stoi(l_val);
        if (lhs == 0) {
            if (op == "&&") {
                irgen.stack_val.push("0");
                return;
            }
        } else {
            if (op == "||") {
                irgen.stack_val.push("1");
                return;
            }
        }
    }
    // shouldn't shortcut l_exp given a const r_exp
    // since l_exp could have side effect

    r_exp->dump_koopa(irgen, out);
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
}

void UnaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
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
}

void PrimaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == PRIMARY_EXP_AST_TYPE_NUMBER) {
        // number
        irgen.stack_val.push(std::to_string(number));
        return;
    } else if (type == PRIMARY_EXP_AST_TYPE_LVAL) {
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
        int val = irgen.symbol_table.get_entry_val(ident);
        irgen.stack_val.push(std::to_string(val));
    } else {
        auto val = irgen.new_val();
        auto aliased_name = irgen.symbol_table.get_aliased_name(ident);
        out << "  " << val << " = load "
            << "@" << aliased_name << std::endl;
        irgen.stack_val.push(val);
    }
}