#include <ast.h>

// helper functions

// check if exp's operand is named or temp symbol
static bool is_symbol(std::string operand) {
    assert(operand != "INVALID");
    auto c = operand[0];
    return c == '%' || c == '@';
}

// Aggregate

typedef enum {
    KOOPA_AGGREGATE_TYPE_INT,
    KOOPA_AGGREGATE_TYPE_UNDEF,
    KOOPA_AGGREGATE_TYPE_AGGREGATE,
    KOOPA_AGGREGATE_TYPE_ZEROINIT,
} koopa_aggregate_type_t;

class KoopaAggregate {
   public:
    koopa_aggregate_type_t type;
    int int_val;
    std::vector<KoopaAggregate> aggs;

    std::string to_string() {
        if (type == KOOPA_AGGREGATE_TYPE_INT) {
            return std::to_string(int_val);
        } else if (type == KOOPA_AGGREGATE_TYPE_UNDEF) {
            return "undef";
        } else if (type == KOOPA_AGGREGATE_TYPE_ZEROINIT) {
            return "zeroinit";
        } else if (type == KOOPA_AGGREGATE_TYPE_AGGREGATE) {
            std::string ret = "{ ";
            auto len = aggs.size();
            for (int i = 0; i < len; i++) {
                ret += aggs[i].to_string();
                if (i + 1 != len) ret += ", ";
            }
            ret += " }";
            return ret;
        } else {
            assert(false);
        }
    }
};

// if begin == end, prod = 1
static int reduce_prod(std::vector<int>::iterator begin,
                       std::vector<int>::iterator end) {
    int prod = 1;
    for (auto it = begin; it != end; it++) {
        prod *= *it;
    }
    return prod;
}

// aggregate a given piece of full array into one reg_agg
static void simplify_aggregate_full_array(
    std::vector<int>::iterator begin, std::vector<int>::iterator end,
    std::vector<int>::iterator it_dim_begin,
    std::vector<int>::iterator it_dim_end, KoopaAggregate &ret_agg) {
    // zeroinit?
    bool is_zeroinit = true;
    for (auto it = begin; it != end; it++)
        is_zeroinit = is_zeroinit && (*it == 0);

    if (is_zeroinit) {
        ret_agg.type = KOOPA_AGGREGATE_TYPE_ZEROINIT;
    } else {
        if (it_dim_begin == it_dim_end) {
            assert(begin + 1 == end);
            ret_agg.type = KOOPA_AGGREGATE_TYPE_INT;
            ret_agg.int_val = *begin;
        } else {
            ret_agg.type = KOOPA_AGGREGATE_TYPE_AGGREGATE;
            auto it_sub_dim_begin = it_dim_begin + 1;
            auto it_sub_dim_end = it_dim_end;
            auto interval = reduce_prod(it_sub_dim_begin, it_sub_dim_end);
            for (int i = 0; i < *it_dim_begin; i++) {
                KoopaAggregate sub_agg;
                simplify_aggregate_full_array(
                    begin + i * interval, begin + (i + 1) * interval,
                    it_sub_dim_begin, it_sub_dim_end, sub_agg);
                ret_agg.aggs.push_back(sub_agg);
            }
            assert(begin + interval * (*it_dim_begin) == end);
        }
    }
}

static void pad_zero_initval_aggregate(IRGenerator &irgen, InitValAST *ast,
                                       std::vector<int>::iterator it_dim_begin,
                                       std::vector<int>::iterator it_dim_end,
                                       std::vector<int> &full_array) {
    assert(ast->type == INIT_VAL_AST_TYPE_SUB_VALS);
    int i = 0;  // current index
    for (auto it_sub_val = ast->init_vals.begin();
         it_sub_val != ast->init_vals.end(); it_sub_val++) {
        auto p_sub_val = dynamic_cast<InitValAST *>(it_sub_val->get());

        auto sub_val_type = p_sub_val->type;
        if (sub_val_type == INIT_VAL_AST_TYPE_EXP) {
            // int, directly insert into full array
            int int_val;
            assert(dynamic_cast<CalcAST *>(p_sub_val->exp.get())
                       ->calc_val(irgen, int_val, true));
            full_array.push_back(int_val);
            i += 1;

        } else {
            // list, check maximum alignment bound
            int prod_dim = 1;  // current maximum boundary
            auto it_sub_dim_begin = it_dim_begin;
            auto it_sub_dim_end = it_dim_end;
            for (; it_sub_dim_begin != it_dim_end; it_sub_dim_begin++) {
                prod_dim = reduce_prod(it_sub_dim_begin, it_dim_end);
                if (i % prod_dim == 0) {
                    break;
                }
            }
            assert(it_sub_dim_end !=
                   it_sub_dim_begin);  // must be on some boundary
            if (it_sub_dim_begin == it_dim_begin) {
                assert(i == 0);  // must be a smaller boundary
                it_sub_dim_begin++;
                assert(it_sub_dim_begin != it_dim_end);
                prod_dim = reduce_prod(it_sub_dim_begin, it_dim_end);
            }
            pad_zero_initval_aggregate(irgen, p_sub_val, it_sub_dim_begin,
                                       it_sub_dim_end, full_array);
            i += prod_dim;
        }
    }
    // pad zero for all the remaining elements
    int prod_dim = reduce_prod(it_dim_begin, it_dim_end);
    for (; i < prod_dim; i++) full_array.push_back(0);
}

// This function promises a valid and simple aggregation result
static void analyze_initval_aggregate(IRGenerator &irgen, InitValAST *ast,
                                      std::vector<int> &dims,
                                      KoopaAggregate &ret_agg) {
    assert(dims.size() != 0);
    std::vector<int> full_array;

    pad_zero_initval_aggregate(irgen, ast, dims.begin(), dims.end(),
                               full_array);
    int prod_dim = reduce_prod(dims.begin(), dims.end());
    assert(full_array.size() == prod_dim);

    simplify_aggregate_full_array(full_array.begin(), full_array.end(),
                                  dims.begin(), dims.end(), ret_agg);
}

// dump koopa

void StartAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    // dump sysy library function
    out << "decl @getint(): i32" << std::endl;
    out << "decl @getch(): i32" << std::endl;
    out << "decl @getarray(*i32): i32" << std::endl;
    out << "decl @putint(i32)" << std::endl;
    out << "decl @putch(i32)" << std::endl;
    out << "decl @putarray(i32, *i32)" << std::endl;
    out << "decl @starttime()" << std::endl;
    out << "decl @stoptime()" << std::endl;
    out << std::endl;

    // add these functions to global symbol table
    irgen.symbol_table.insert_func_entry("getint", "int", std::vector<bool>());
    irgen.symbol_table.insert_func_entry("getch", "int", std::vector<bool>());
    irgen.symbol_table.insert_func_entry("getarray", "int",
                                         std::vector<bool>({true}));
    irgen.symbol_table.insert_func_entry("putint", "void",
                                         std::vector<bool>({false}));
    irgen.symbol_table.insert_func_entry("putch", "void",
                                         std::vector<bool>({false}));
    irgen.symbol_table.insert_func_entry("putarray", "void",
                                         std::vector<bool>({false, true}));
    irgen.symbol_table.insert_func_entry("starttime", "void",
                                         std::vector<bool>());
    irgen.symbol_table.insert_func_entry("stoptime", "void",
                                         std::vector<bool>());

    // the actual dumping order is reverse!
    for (auto it = units.rbegin(); it != units.rend(); it++) {
        it->get()->dump_koopa(irgen, out);
        out << std::endl;
    }
}

void CompUnitAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == COMP_UNIT_AST_TYPE_FUNC) {
        assert(func_def.get() != nullptr);
        func_def->dump_koopa(irgen, out);
    } else if (type == COMP_UNIT_AST_TYPE_DECL) {
        assert(decl.get() != nullptr);
        decl->dump_koopa(irgen, out);
    } else {
        assert(false);
    }
}

void DeclAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    for (auto &def : defs) def->dump_koopa(irgen, out);
}

void DeclDefAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (indexes.size() == 0) {  // var
        if (is_const) {
            // add const entry into symbol table
            int const_entry_val;
            auto exp = dynamic_cast<InitValAST *>(init_val.get())->exp.get();
            assert(dynamic_cast<CalcAST *>(exp)->calc_val(
                irgen, const_entry_val, true));
            irgen.symbol_table.insert_const_var_entry(ident, const_entry_val);
        } else {
            irgen.symbol_table.insert_var_entry(ident);
            if (irgen.symbol_table.is_global_symbol_table()) {
                std::string store_val = "zeroinit";
                if (init_val.get()) {
                    auto exp =
                        dynamic_cast<InitValAST *>(init_val.get())->exp.get();
                    // global decl only use const
                    int exp_val;
                    assert(dynamic_cast<CalcAST *>(exp)->calc_val(
                        irgen, exp_val, true));
                    store_val = std::to_string(exp_val);
                }

                auto var_name = irgen.symbol_table.get_var_name(ident);
                out << "global " << var_name << " = alloc i32, " << store_val
                    << std::endl;

            } else {
                // store initial value to memory, if there is
                std::string store_val;
                if (init_val.get()) {
                    auto exp =
                        dynamic_cast<InitValAST *>(init_val.get())->exp.get();
                    // local decl could use variables
                    exp->dump_koopa(irgen, out);
                    store_val = irgen.stack_val.top();
                    irgen.stack_val.pop();
                }

                auto var_name = irgen.symbol_table.get_var_name(ident);
                out << "  " << var_name << " = alloc i32" << std::endl;
                if (init_val.get()) {
                    out << "  store " << store_val << ", " << var_name
                        << std::endl;
                }
            }
        }
    } else {  // array
        // get array dims
        std::vector<int> dims;
        for (auto it_index = indexes.begin(); it_index != indexes.end();
             it_index++) {
            int dim;
            assert(dynamic_cast<CalcAST *>(it_index->get())
                       ->calc_val(irgen, dim, true));
            dims.push_back(dim);
        }

        // add to symbol table
        irgen.symbol_table.insert_array_entry(ident, dims);

        // infer array type
        std::string array_type = "i32";
        for (auto it = dims.rbegin(); it != dims.rend(); it++) {
            array_type = "[" + array_type + ", " + std::to_string(*it) + "]";
        }

        // global alloc / local alloc
        if (irgen.symbol_table.is_global_symbol_table()) {
            auto array_name = irgen.symbol_table.get_array_name(ident);
            out << "global " << array_name << " = alloc " << array_type;
            if (init_val.get()) {
                KoopaAggregate agg;
                analyze_initval_aggregate(
                    irgen, dynamic_cast<InitValAST *>(init_val.get()), dims,
                    agg);
                out << ", " << agg.to_string();
            } else {
                out << ", zeroinit" << std::endl;
            }
            out << std::endl;

        } else {
            auto array_name = irgen.symbol_table.get_array_name(ident);
            out << "  " << array_name << " = alloc " << array_type << std::endl;
            if (init_val.get()) {
                KoopaAggregate agg;
                analyze_initval_aggregate(
                    irgen, dynamic_cast<InitValAST *>(init_val.get()), dims,
                    agg);
                out << "  store " << agg.to_string() << ", " << array_name
                    << std::endl;
            }
        }
    }
}

void FuncDefAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    out << "fun @" << ident << "(";
    irgen.symbol_table.push_block();

    // dump param list
    std::vector<bool> is_func_param_ptr;
    int cnt_param = 0;
    for (auto &param_ : params) {
        auto param = (FuncFParamAST *)(param_.get());

        // record param ptr status
        is_func_param_ptr.push_back(param->is_ptr);

        std::string param_name;
        std::string param_type;
        if (param->is_ptr) {
            // infer param type
            std::vector<int> dims;
            for (auto it_index = param->indexes.begin();
                 it_index != param->indexes.end(); it_index++) {
                int dim;
                dynamic_cast<CalcAST *>(it_index->get())
                    ->calc_val(irgen, dim, true);
                dims.push_back(dim);
            }

            irgen.symbol_table.insert_array_entry(param->ident, dims, true);
            param_name = irgen.symbol_table.get_array_name(param->ident);
            param_type = irgen.symbol_table.get_array_entry_type(param->ident);

        } else {
            irgen.symbol_table.insert_var_entry(param->ident);
            param_name = irgen.symbol_table.get_var_name(param->ident);
            param_type = "i32";
        }
        out << "@" << param_name.c_str() + 1;
        out << ": " << param_type;
        if (++cnt_param < params.size()) out << ", ";
    }
    out << ")";

    // dump func type
    if (func_type == "int") {
        out << ": i32 ";
    } else if (func_type == "void") {
    } else {
        std::cerr << "FuncDefAST: invalid func type: " << func_type
                  << std::endl;
        assert(false);
    }
    // register func name and func param type before dumping func body
    irgen.symbol_table.insert_func_entry(ident, func_type, is_func_param_ptr);

    // prepare the first basic block for control flow
    out << "{" << std::endl;
    auto block_name = irgen.new_block();
    irgen.control_flow.init_entry_block(block_name, out);

    // duplicate formal parameters
    cnt_param = 0;
    for (auto &param_ : params) {
        auto param = (FuncFParamAST *)(param_.get());
        std::string param_name;
        std::string param_type;
        if (is_func_param_ptr[cnt_param]) {
            param_name = irgen.symbol_table.get_array_name(param->ident);
            param_type = irgen.symbol_table.get_array_entry_type(param->ident);
        } else {
            param_name = irgen.symbol_table.get_var_name(param->ident);
            param_type = "i32";
        }
        out << "  " << param_name << " = alloc " << param_type << std::endl;
        out << "  store @" << param_name.c_str() + 1 << ", " << param_name
            << std::endl;
        cnt_param++;
    }

    block->dump_koopa(irgen, out);
    irgen.symbol_table.pop_block();

    // check if basic block has returned, and give up control flow
    if (irgen.control_flow.check_ending_status() ==
        BASIC_BLOCK_ENDING_STATUS_NULL) {
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_RETURN);
        if (func_type == "int") {
            out << "  ret 1919810" << std::endl;
        } else if (func_type == "void") {
            out << "  ret" << std::endl;
        } else {
            assert(false);
        }
    }
    irgen.control_flow.cur_block = "";

    out << "}" << std::endl;
}

void BlockAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    for (auto &item : items) {
        // if block has ending status, early exit.
        auto status = irgen.control_flow.check_ending_status();
        if (status != BASIC_BLOCK_ENDING_STATUS_NULL) break;
        item->dump_koopa(irgen, out);
    }
}

void BlockItemAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    item->dump_koopa(irgen, out);
}

// dump next basic block while taking care of control flow
static void dump_next_basic_block(BaseAST *stmt, std::string cur_block,
                                  std::string end_block, IRGenerator &irgen,
                                  std::ostream &out) {
    // switch to current block
    assert(irgen.control_flow.switch_control_flow(cur_block, out));
    // dump to current block
    stmt->dump_koopa(irgen, out);
    // jump to ending when current block hasn't finished
    if (irgen.control_flow.check_ending_status() ==
        BASIC_BLOCK_ENDING_STATUS_NULL) {
        out << "  jump " << end_block << std::endl;
        irgen.control_flow.modify_ending_status(BASIC_BLOCK_ENDING_STATUS_JUMP);
        irgen.control_flow.add_control_edge(end_block);
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
        auto lval_type = irgen.symbol_table.get_entry_type(lval_name);
        if (lval_type == SYMBOL_TABLE_ENTRY_VAR) {
            assert(!irgen.symbol_table.is_const_var_entry(lval_name));
            auto lval_var_name = irgen.symbol_table.get_var_name(lval_name);
            out << "  store " << r_val << ", " << lval_var_name << std::endl;
        } else if (lval_type == SYMBOL_TABLE_ENTRY_ARRAY) {
            dynamic_cast<LValAST *>(lval.get())
                ->dump_koopa_parse_indexes(irgen, out);
            std::string ptr_index = irgen.stack_val.top();
            irgen.stack_val.pop();
            out << "  store " << r_val << ", " << ptr_index << std::endl;
        } else {
            std::cerr << "StmtAST: invalid lval type!" << std::endl;
            assert(false);
        }

    } else if (type == STMT_AST_TYPE_RETURN) {
        if (exp.get() != nullptr) {
            exp->dump_koopa(irgen, out);
            auto ret_val = irgen.stack_val.top();
            irgen.stack_val.pop();
            out << "  ret " << ret_val << std::endl;
        } else {
            out << "  ret" << std::endl;
        }
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_RETURN);  // block should return

    } else if (type == STMT_AST_TYPE_EXP) {
        if (exp.get() != nullptr) {
            exp->dump_koopa(irgen, out);
            irgen.stack_val.pop();  // no one use it
        }

    } else if (type == STMT_AST_TYPE_BLOCK) {
        irgen.symbol_table.push_block();
        block->dump_koopa(irgen, out);
        irgen.symbol_table.pop_block();

    } else if (type == STMT_AST_TYPE_IF) {
        // dump condition expression
        assert(exp.get() != nullptr);
        exp->dump_koopa(irgen, out);
        auto cond = irgen.stack_val.top();
        irgen.stack_val.pop();
        // TODO: if cond is pre-determined, bypass the following procedure

        // generate then, else, end basic blocks
        assert(then_stmt.get() != nullptr);
        if (else_stmt.get() != nullptr) {
            auto then_block_name = irgen.new_block();
            auto end_block_name = irgen.new_block();
            auto else_block_name = irgen.new_block();

            // finish current block
            irgen.control_flow.modify_ending_status(
                BASIC_BLOCK_ENDING_STATUS_BRANCH);
            out << "  br " << cond << ", " << then_block_name << ", "
                << else_block_name << std::endl;

            irgen.control_flow.insert_if_else(then_block_name, else_block_name,
                                              end_block_name);

            dump_next_basic_block(then_stmt.get(), then_block_name,
                                  end_block_name, irgen, out);
            dump_next_basic_block(else_stmt.get(), else_block_name,
                                  end_block_name, irgen, out);

            irgen.control_flow.switch_control_flow(end_block_name, out);

        } else {
            auto then_block_name = irgen.new_block();
            auto end_block_name = irgen.new_block();

            // finish current block
            irgen.control_flow.modify_ending_status(
                BASIC_BLOCK_ENDING_STATUS_BRANCH);
            out << "  br " << cond << ", " << then_block_name << ", "
                << end_block_name << std::endl;

            irgen.control_flow.insert_if(then_block_name, end_block_name);

            dump_next_basic_block(then_stmt.get(), then_block_name,
                                  end_block_name, irgen, out);

            irgen.control_flow.switch_control_flow(end_block_name, out);
        }

    } else if (type == STMT_AST_TYPE_WHILE) {
        assert(exp.get() != nullptr);
        assert(do_stmt.get() != nullptr);

        // generate entry, body, end block
        auto entry_block_name = irgen.new_block();
        auto body_block_name = irgen.new_block();
        auto end_block_name = irgen.new_block();

        irgen.control_flow.insert_while(entry_block_name, body_block_name,
                                        end_block_name);

        // finish current block
        out << "  jump " << entry_block_name << std::endl;
        irgen.control_flow.modify_ending_status(BASIC_BLOCK_ENDING_STATUS_JUMP);

        // dump entry block
        assert(irgen.control_flow.switch_control_flow(entry_block_name, out));

        exp->dump_koopa(irgen, out);
        auto cond = irgen.stack_val.top();
        irgen.stack_val.pop();
        // TODO: if cond is pre-determined, bypass the following procedure

        out << "  br " << cond << ", " << body_block_name << ", "
            << end_block_name << std::endl;
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_BRANCH);

        // dump body block, same as if-then-else stmt
        dump_next_basic_block(do_stmt.get(), body_block_name, entry_block_name,
                              irgen, out);

        // dump end block
        assert(irgen.control_flow.switch_control_flow(end_block_name, out));

    } else if (type == STMT_AST_TYPE_BREAK) {
        irgen.control_flow._break(out);

    } else if (type == STMT_AST_TYPE_CONTINUE) {
        irgen.control_flow._continue(out);

    } else {
        assert(false);
    }
}

void ExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    binary_exp->dump_koopa(irgen, out);
    return;  // needless to operate on stack
}

void BinaryExpAST::dump_koopa_land_lor(IRGenerator &irgen,
                                       std::ostream &out) const {
    assert(op == "&&" || op == "||");

    // dump lhs first
    l_exp->dump_koopa(irgen, out);
    auto l_val = irgen.stack_val.top();
    irgen.stack_val.pop();

    if (!is_symbol(l_val)) {
        auto lhs = std::stoi(l_val);
        if (op == "&&" && lhs == 0) {
            irgen.stack_val.push("0");
            return;
        }
        if (op == "||" && lhs != 0) {
            irgen.stack_val.push("1");
            return;
        }

        // Knowing that lhs is const we directly dump rhs
        r_exp->dump_koopa(irgen, out);
        auto r_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        if (!is_symbol(r_val)) {
            auto rhs = std::stoi(r_val);
            if (rhs == 0) {
                irgen.stack_val.push("0");
                return;
            } else {
                irgen.stack_val.push("1");
                return;
            }

        } else {
            // rhs is variable, we check if it's non-zero
            auto lr_val = irgen.new_val();
            out << "  " << lr_val << " = ne " << r_val << ", 0" << std::endl;
            irgen.stack_val.push(lr_val);  // needless to &&
            return;
        }

    } else {
        // not a symbol. prepare return val first
        // &&: exp_val = l_val ? r_val != 0 : 0;
        // ||: exp_val = l_val ? 1 : r_val != 0;
        auto exp_val = irgen.new_val();
        out << "  " << exp_val << " = alloc i32" << std::endl;
        if (op == "&&")
            out << "  store " << 0 << ", " << exp_val << std::endl;
        else
            out << "  store " << 1 << ", " << exp_val << std::endl;

        // similar to if-then-end stmt
        auto then_block_name = irgen.new_block();
        auto end_block_name = irgen.new_block();

        // finish current block
        irgen.control_flow.modify_ending_status(
            BASIC_BLOCK_ENDING_STATUS_BRANCH);
        if (op == "&&")
            out << "  br " << l_val << ", " << then_block_name << ", "
                << end_block_name << std::endl;  // l != 0 ? r : 0;
        else
            out << "  br " << l_val << ", " << end_block_name << ", "
                << then_block_name << std::endl;  // l != 0 ? 1 : r;

        irgen.control_flow.insert_if(then_block_name, end_block_name);
        assert(irgen.control_flow.switch_control_flow(then_block_name, out));

        r_exp->dump_koopa(irgen, out);
        auto r_val = irgen.stack_val.top();
        irgen.stack_val.pop();

        auto lr_val = irgen.new_val();
        out << "  " << lr_val << " = ne " << r_val << ", 0" << std::endl;
        out << "  store " << lr_val << ", " << exp_val << std::endl;

        assert(irgen.control_flow.check_ending_status() ==
               BASIC_BLOCK_ENDING_STATUS_NULL);
        out << "  jump " << end_block_name << std::endl;
        irgen.control_flow.modify_ending_status(BASIC_BLOCK_ENDING_STATUS_JUMP);
        irgen.control_flow.add_control_edge(end_block_name);

        assert(irgen.control_flow.switch_control_flow(end_block_name, out));

        // load to register
        auto ret_val = irgen.new_val();
        out << "  " << ret_val << " = load " << exp_val << std::endl;
        irgen.stack_val.push(ret_val);
        return;
    }
}

void BinaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (op == "&&" || op == "||") {
        dump_koopa_land_lor(irgen, out);
        return;
    }

    l_exp->dump_koopa(irgen, out);
    auto l_val = irgen.stack_val.top();
    irgen.stack_val.pop();

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
        else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);
        }
        irgen.stack_val.push(std::to_string(ret));
        return;
    }

    // dump exp w.r.t. op
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
    } else {
        std::cerr << "Invalid op: " << op << std::endl;
        assert(false);
    }
    out << l_val << ", " << r_val << std::endl;
    irgen.stack_val.push(exp_val);
}

void UnaryExpAST::dump_koopa(IRGenerator &irgen, std::ostream &out) const {
    if (type == UNARY_EXP_AST_TYPE_OP) {
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
    } else if (type == UNARY_EXP_AST_TYPE_FUNC) {
        // dump all the exp
        std::vector<std::string> rparams;
        int cnt_param = 0;
        for (auto &param : params) {
            if (irgen.symbol_table.is_func_param_ptr(ident, cnt_param)) {
                auto exp = dynamic_cast<ExpAST *>(param.get());
                assert(exp);
                auto prim_exp =
                    dynamic_cast<PrimaryExpAST *>(exp->binary_exp.get());
                assert(prim_exp);
                assert(prim_exp->type == PRIMARY_EXP_AST_TYPE_LVAL);
                auto lval_exp = dynamic_cast<LValAST *>(prim_exp->lval.get());
                lval_exp->dump_koopa_parse_indexes(irgen, out);

                // get first element ptr
                auto ptr_arr = irgen.stack_val.top();
                irgen.stack_val.pop();
                auto ptr_first_elem = irgen.new_val();
                if (lval_exp->indexes.size() == 0 &&
                    irgen.symbol_table.is_ptr_array_entry(lval_exp->ident)) {
                    auto ptr_ttmp = irgen.new_val();
                    out << "  " << ptr_ttmp << " = load " << ptr_arr
                        << std::endl;
                    out << "  " << ptr_first_elem << " = getptr " << ptr_ttmp
                        << ", 0" << std::endl;
                } else {
                    out << "  " << ptr_first_elem << " = getelemptr " << ptr_arr
                        << ", 0" << std::endl;
                }
                irgen.stack_val.push(ptr_first_elem);

            } else {
                param->dump_koopa(irgen, out);
            }
            rparams.push_back(irgen.stack_val.top());
            irgen.stack_val.pop();

            cnt_param++;
        }

        assert(ident != "");
        auto func_type = irgen.symbol_table.get_func_entry_type(ident);
        if (func_type == "int") {
            auto ret_val = irgen.new_val();
            out << "  " << ret_val << " = ";
            irgen.stack_val.push(ret_val);
        } else if (func_type == "void") {
            out << "  ";
            irgen.stack_val.push("INVALID");  // keep consistent with other exp
        } else {
            std::cerr << "Unknown func type: " << func_type << std::endl;
        }
        out << "call @" << ident << "(";
        cnt_param = 0;
        for (auto &param : rparams) {
            out << param;
            if (++cnt_param != rparams.size()) out << ", ";
        }
        out << ")" << std::endl;

    } else {
        std::cerr << "Invalid unary exp type: " << type << std::endl;
        assert(false);
    }
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
    auto type = irgen.symbol_table.get_entry_type(ident);
    if (type == SYMBOL_TABLE_ENTRY_VAR) {
        if (irgen.symbol_table.is_const_var_entry(ident)) {
            int val = irgen.symbol_table.get_const_var_val(ident);
            irgen.stack_val.push(std::to_string(val));
        } else {
            auto val = irgen.new_val();
            auto aliased_name = irgen.symbol_table.get_var_name(ident);
            out << "  " << val << " = load " << aliased_name << std::endl;
            irgen.stack_val.push(val);
        }
    } else if (type == SYMBOL_TABLE_ENTRY_ARRAY) {
        dump_koopa_parse_indexes(irgen, out);
        std::string ptr_index = irgen.stack_val.top();
        irgen.stack_val.pop();
        std::string val_name = irgen.new_val();
        out << "  " << val_name << " = load " << ptr_index << std::endl;
        irgen.stack_val.push(val_name);
    } else {
        std::cerr << "LValAST: invalid type!" << std::endl;
        assert(false);
    }
}

void LValAST::dump_koopa_parse_indexes(IRGenerator &irgen,
                                       std::ostream &out) const {
    // array could be partially parsed
    std::string aliased_name = irgen.symbol_table.get_array_name(ident);
    std::string ptr_index = aliased_name;
    for (auto it_index = indexes.begin(); it_index != indexes.end();
         it_index++) {
        // dump the index (not necessarily const)
        it_index->get()->dump_koopa(irgen, out);
        auto dim = irgen.stack_val.top();
        irgen.stack_val.pop();
        std::string ptr_tmp = irgen.new_val();
        if (it_index == indexes.begin() &&
            irgen.symbol_table.is_ptr_array_entry(ident)) {
            auto ptr_ttmp = irgen.new_val();
            out << "  " << ptr_ttmp << " = load " << ptr_index << std::endl;
            out << "  " << ptr_tmp << " = getptr " << ptr_ttmp << ", " << dim
                << std::endl;
        } else {
            out << "  " << ptr_tmp << " = getelemptr " << ptr_index << ", "
                << dim << std::endl;
        }
        ptr_index = ptr_tmp;
    }
    irgen.stack_val.push(ptr_index);
}