#include <tcgen.h>

TargetCodeGenerator::TargetCodeGenerator(const char *koopa_file,
                                         std::ostream &out)
    : out{out} {
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_file(koopa_file, &program);
    assert(ret == KOOPA_EC_SUCCESS);
    builder = koopa_new_raw_program_builder();
    raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
}

TargetCodeGenerator::~TargetCodeGenerator() {
    koopa_delete_raw_program_builder(builder);
}

int TargetCodeGenerator::dump_riscv() {
    int ret;
    ret = dump_koopa_raw_slice(raw.values);
    ret = dump_koopa_raw_slice(raw.funcs);
    return ret;
}

// some useful debugging function
std::string to_koopa_raw_value_tag(koopa_raw_value_tag_t tag) {
    switch (tag) {
        case KOOPA_RVT_INTEGER:
            return "KOOPA_RVT_INTEGER";
        case KOOPA_RVT_ZERO_INIT:
            return "KOOPA_RVT_ZERO_INIT";
        case KOOPA_RVT_UNDEF:
            return "KOOPA_RVT_UNDEF";
        case KOOPA_RVT_AGGREGATE:
            return "KOOPA_RVT_AGGREGATE";
        case KOOPA_RVT_FUNC_ARG_REF:
            return "KOOPA_RVT_FUNC_ARG_REF";
        case KOOPA_RVT_BLOCK_ARG_REF:
            return "KOOPA_RVT_BLOCK_ARG_REF";
        case KOOPA_RVT_ALLOC:
            return "KOOPA_RVT_ALLOC";
        case KOOPA_RVT_GLOBAL_ALLOC:
            return "KOOPA_RVT_GLOBAL_ALLOC";
        case KOOPA_RVT_LOAD:
            return "KOOPA_RVT_LOAD";
        case KOOPA_RVT_STORE:
            return "KOOPA_RVT_STORE";
        case KOOPA_RVT_GET_PTR:
            return "KOOPA_RVT_GET_PTR";
        case KOOPA_RVT_GET_ELEM_PTR:
            return "KOOPA_RVT_GET_ELEM_PTR";
        case KOOPA_RVT_BINARY:
            return "KOOPA_RVT_BINARY";
        case KOOPA_RVT_BRANCH:
            return "KOOPA_RVT_BRANCH";
        case KOOPA_RVT_JUMP:
            return "KOOPA_RVT_JUMP";
        case KOOPA_RVT_CALL:
            return "KOOPA_RVT_CALL";
        case KOOPA_RVT_RETURN:
            return "KOOPA_RVT_RETURN";
        default:
            return "INVALID KOOPA RVT TAG!";
    }
}

std::string to_koopa_raw_type_tag(koopa_raw_type_tag_t tag) {
    switch (tag) {
        case KOOPA_RTT_INT32:
            return "KOOPA_RTT_INT32";
        case KOOPA_RTT_UNIT:
            return "KOOPA_RTT_UNIT";
        case KOOPA_RTT_ARRAY:
            return "KOOPA_RTT_ARRAY";
        case KOOPA_RTT_POINTER:
            return "KOOPA_RTT_POINTER";
        case KOOPA_RTT_FUNCTION:
            return "KOOPA_RTT_FUNCTION";
        default:
            return "INVALID KOOPA RTT TAG!";
    }
}

// dump riscv inst

void TargetCodeGenerator::dump_riscv_inst(std::string inst,
                                          std::string reg_0 = "",
                                          std::string reg_1 = "",
                                          std::string reg_2 = "") {
    out << "  ";
    out << std::left << std::setw(6) << inst;
    if (reg_0 == "")
        out << std::endl;
    else {
        out << reg_0;
        if (reg_1 == "")
            out << std::endl;
        else {
            out << ", " << reg_1;
            if (reg_2 == "")
                out << std::endl;
            else
                out << ", " << reg_2 << std::endl;
        }
    }
}

void TargetCodeGenerator::dump_lw(std::string reg, int offset,
                                  std::string base = "sp") {
    if (offset <= 2047 && offset >= -2048) {
        dump_riscv_inst("lw", reg, std::to_string(offset) + "(" + base + ")");
    } else {
        // take an empty register for offset
        // TODO: before considering register allocation, we use t6
        dump_riscv_inst("li", "t6", std::to_string(offset));
        dump_riscv_inst("add", "t6", "t6", "sp");
        dump_riscv_inst("lw", reg, "0(t6)");
    }
}

void TargetCodeGenerator::dump_sw(std::string reg, int offset) {
    if (offset <= 2047 && offset >= -2048) {
        dump_riscv_inst("sw", reg, std::to_string(offset) + "(sp)");
    } else {
        // take an empty register for offset
        // TODO: before considering register allocation, we use t6
        dump_riscv_inst("li", "t6", std::to_string(offset));
        dump_riscv_inst("add", "t6", "t6", "sp");
        dump_riscv_inst("sw", reg, "0(t6)");
    }
}

// dump koopa data structure

int TargetCodeGenerator::dump_koopa_raw_slice(koopa_raw_slice_t slice) {
    for (size_t i = 0; i < slice.len; i++) {
        const void *p = slice.buffer[i];
        int ret;
        if (slice.kind == KOOPA_RSIK_FUNCTION)
            ret = dump_koopa_raw_function((koopa_raw_function_t)p);
        else if (slice.kind == KOOPA_RSIK_VALUE)
            // we should only reference inst array.
            ret = dump_koopa_raw_value((koopa_raw_value_t)p);
        else if (slice.kind == KOOPA_RSIK_BASIC_BLOCK)
            ret = dump_koopa_raw_basic_block((koopa_raw_basic_block_t)p);
        else
            ret = -1;
        assert(!ret);
    }
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_function(koopa_raw_function_t func) {
    if (func->bbs.len == 0) {
        return 0;  // func decl, should be ignored
    }

    // function statement

    // function name, ignore first character
    out << "  .text" << std::endl;
    out << "  .globl " << func->name + 1 << std::endl;
    out << func->name + 1 << ":" << std::endl;

    runtime_stack.push(StackFrame(func));

    // prologue
    // set up stack frame
    int frame_length = runtime_stack.top().get_length();
    if (frame_length <= 2048)
        dump_riscv_inst("addi", "sp", "sp", std::to_string(-frame_length));
    else {
        dump_riscv_inst("li", "t0", std::to_string(-frame_length));
        dump_riscv_inst("add", "sp", "sp", "t0");
        // length will never be used, so everyone can use t0 later
    }
    // save callee registers
    for (auto &it : runtime_stack.top().saved_registers) {
        auto reg = it.first;
        auto offset = it.second.offset;
        dump_sw(reg, offset);
    }
    // set up s0  TODO: only > 8 func param need this
    if (frame_length <= 2048)
        dump_riscv_inst("addi", "s0", "sp", std::to_string(frame_length));
    else {
        dump_riscv_inst("li", "t0", std::to_string(frame_length));
        dump_riscv_inst("add", "s0", "sp", "t0");
    }

    int ret = dump_koopa_raw_slice(func->bbs);
    runtime_stack.pop();
    out << std::endl;

    return ret;
}

int TargetCodeGenerator::dump_koopa_raw_basic_block(
    koopa_raw_basic_block_t bb) {
    out << bb->name + 1 << ":" << std::endl;
    int ret = dump_koopa_raw_slice(bb->insts);
    return ret;
}

int TargetCodeGenerator::dump_koopa_raw_value(koopa_raw_value_t value) {
    auto tag = value->kind.tag;
    if (tag == KOOPA_RVT_BINARY)
        return dump_koopa_raw_value_binary(value);
    else if (tag == KOOPA_RVT_RETURN)
        return dump_koopa_raw_value_return(value);
    else if (tag == KOOPA_RVT_ALLOC)
        return 0;  // needless to dump
    else if (tag == KOOPA_RVT_GLOBAL_ALLOC)
        return dump_koopa_raw_value_global_alloc(value);
    else if (tag == KOOPA_RVT_LOAD)
        return dump_koopa_raw_value_load(value);
    else if (tag == KOOPA_RVT_STORE)
        return dump_koopa_raw_value_store(value);
    else if (tag == KOOPA_RVT_BRANCH)
        return dump_koopa_raw_value_branch(value);
    else if (tag == KOOPA_RVT_JUMP)
        return dump_koopa_raw_value_jump(value);
    else if (tag == KOOPA_RVT_CALL)
        return dump_koopa_raw_value_call(value);
    else {
        std::cerr << "Raw value type " << to_koopa_raw_value_tag(tag)
                  << " not implemented." << std::endl;
        return -1;
    }
}

// dump koopa raw value

int TargetCodeGenerator::dump_koopa_raw_value_binary(koopa_raw_value_t value) {
    auto op = value->kind.data.binary.op;
    std::string lhs, rhs;
    // t0 = t1 op t2

    // find lhs operand, assign a register if necessary
    auto lhs_val = value->kind.data.binary.lhs;
    if (lhs_val->kind.tag == KOOPA_RVT_BINARY) {
        // binary: load from stack
        auto offset = runtime_stack.top().get_offset(lhs_val);
        dump_lw("t1", offset);
        lhs = "t1";
    } else if (lhs_val->kind.tag == KOOPA_RVT_INTEGER) {
        auto lhs_int = lhs_val->kind.data.integer.value;
        dump_riscv_inst("li", "t1", std::to_string(lhs_int));
        lhs = "t1";
    } else if (lhs_val->kind.tag == KOOPA_RVT_LOAD) {
        auto offset = runtime_stack.top().get_offset(lhs_val);
        dump_lw("t1", offset);
        lhs = "t1";
    } else if (lhs_val->kind.tag == KOOPA_RVT_CALL) {
        auto offset = runtime_stack.top().get_offset(lhs_val);
        dump_lw("t1", offset);
        lhs = "t1";
    } else {
        auto tag = to_koopa_raw_value_tag(lhs_val->kind.tag);
        std::cerr << "BINARY val type: " << tag << std::endl;
        assert(false);
    }

    // find rhs operand
    // imm format is available for andi/ori/xori/addi
    auto rhs_val = value->kind.data.binary.rhs;
    bool rhs_imm = false;  // TODO: simplify rhs imm
    if (rhs_val->kind.tag == KOOPA_RVT_BINARY) {
        // binary: load from stack
        auto offset = runtime_stack.top().get_offset(rhs_val);
        dump_lw("t2", offset);
        rhs = "t2";
    } else if (rhs_val->kind.tag == KOOPA_RVT_INTEGER) {
        auto rhs_int = rhs_val->kind.data.integer.value;
        dump_riscv_inst("li", "t2", std::to_string(rhs_int));
        rhs = "t2";
    } else if (rhs_val->kind.tag == KOOPA_RVT_LOAD) {
        auto offset = runtime_stack.top().get_offset(rhs_val);
        dump_lw("t2", offset);
        rhs = "t2";
    } else if (rhs_val->kind.tag == KOOPA_RVT_CALL) {
        auto offset = runtime_stack.top().get_offset(rhs_val);
        dump_lw("t2", offset);
        rhs = "t2";
    } else {
        std::cerr << "BINARY val type: " << rhs_val->kind.tag << std::endl;
        assert(false);
    }

    // given op type, dump the value
    auto reg = "t0";
    if (op == KOOPA_RBO_NOT_EQ) {
        dump_riscv_inst("xor", reg, lhs, rhs);
        dump_riscv_inst("snez", reg, reg);
    } else if (op == KOOPA_RBO_EQ) {
        dump_riscv_inst("xor", reg, lhs, rhs);
        dump_riscv_inst("seqz", reg, reg);
    } else if (op == KOOPA_RBO_GT) {
        dump_riscv_inst("sgt", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_LT) {
        dump_riscv_inst("slt", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_GE) {  // not less than
        dump_riscv_inst("slt", reg, lhs, rhs);
        dump_riscv_inst("xori", reg, reg, "1");
    } else if (op == KOOPA_RBO_LE) {  // not greater than
        dump_riscv_inst("sgt", reg, lhs, rhs);
        dump_riscv_inst("xori", reg, reg, "1");
    } else if (op == KOOPA_RBO_ADD) {
        std::string inst = "add";
        if (rhs_imm) inst += "i";
        dump_riscv_inst(inst, reg, lhs, rhs);
    } else if (op == KOOPA_RBO_SUB) {
        dump_riscv_inst("sub", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_MUL) {
        dump_riscv_inst("mul", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_DIV) {
        dump_riscv_inst("div", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_MOD) {
        dump_riscv_inst("rem", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_AND) {
        std::string inst = "and";
        if (rhs_imm) inst += "i";
        dump_riscv_inst(inst, reg, lhs, rhs);
    } else if (op == KOOPA_RBO_OR) {
        std::string inst = "or";
        if (rhs_imm) inst += "i";
        dump_riscv_inst(inst, reg, lhs, rhs);
    } else if (op == KOOPA_RBO_XOR) {
        std::string inst = "xor";
        if (rhs_imm) inst += "i";
        dump_riscv_inst(inst, reg, lhs, rhs);
    } else if (op == KOOPA_RBO_SHL) {
        dump_riscv_inst("shl", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_SHR) {
        dump_riscv_inst("shr", reg, lhs, rhs);
    } else if (op == KOOPA_RBO_SAR) {
        dump_riscv_inst("sar", reg, lhs, rhs);
    } else {
        std::cerr << "Invalid operator for binary inst." << std::endl;
        assert(false);
    }

    // write the result
    auto offset = runtime_stack.top().get_offset(value);
    dump_sw(reg, offset);

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_load(koopa_raw_value_t value) {
    // store the return value back on stack, kind of stupid...
    auto src = value->kind.data.load.src;
    if (src->kind.tag == KOOPA_RVT_STORE) {
        auto src_offset = runtime_stack.top().get_offset(src);
        dump_lw("t0", src_offset);
    } else if (src->kind.tag == KOOPA_RVT_ALLOC) {
        auto src_offset = runtime_stack.top().get_offset(src);
        dump_lw("t0", src_offset);
    } else if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        dump_riscv_inst("la", "t0", src->name + 1);
        dump_riscv_inst("lw", "t0", "0(t0)");
    } else {
        auto tag = to_koopa_raw_value_tag(src->kind.tag);
        std::cerr << "Load: invalid type " << tag << std::endl;
        assert(false);
    }

    // store onto stack
    auto dst_offset = runtime_stack.top().get_offset(value);
    dump_sw("t0", dst_offset);
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_store(koopa_raw_value_t value) {
    // load value from stack, and put back to dest ...
    auto src = value->kind.data.store.value;
    if (src->kind.tag == KOOPA_RVT_INTEGER) {
        int src_val = src->kind.data.integer.value;
        dump_riscv_inst("li", "t0", std::to_string(src_val));
    } else if (src->kind.tag == KOOPA_RVT_BINARY) {
        auto offset = runtime_stack.top().get_offset(src);
        dump_lw("t0", offset);
    } else if (src->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        auto index = src->kind.data.func_arg_ref.index;
        if (index < 8) {
            // this, of course, could be optimized
            std::string reg = "a" + std::to_string(index);
            dump_riscv_inst("mv", "t0", reg);
        } else {
            // prologue should set up s0
            int offset = (index - 8) * 4;
            dump_lw("t0", offset, "s0");
        }
    } else if (src->kind.tag == KOOPA_RVT_CALL) {
        // func call inst should put result back on stack
        auto offset = runtime_stack.top().get_offset(src);
        dump_lw("t0", offset);
    } else {
        auto tag = to_koopa_raw_value_tag(src->kind.tag);
        std::cerr << "STORE src type: " << tag << std::endl;
        assert(false);
    }

    // dump to dst
    auto dst = value->kind.data.store.dest;
    if (dst->kind.tag == KOOPA_RVT_ALLOC) {
        auto dst_offset = runtime_stack.top().get_offset(dst);
        dump_sw("t0", dst_offset);
    } else if (dst->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        dump_riscv_inst("la", "t4", dst->name + 1);
        dump_riscv_inst("sw", "t0", "0(t4)");
    } else {
        auto tag = to_koopa_raw_value_tag(src->kind.tag);
        std::cerr << "Store: invalid type " << tag << std::endl;
        assert(false);
    }
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_return(koopa_raw_value_t value) {
    // move operand to a0
    auto ret_val = value->kind.data.ret.value;
    if (ret_val != nullptr) {
        if (ret_val->kind.tag == KOOPA_RVT_BINARY) {
            auto offset = runtime_stack.top().get_offset(ret_val);
            dump_lw("a0", offset);
        } else if (ret_val->kind.tag == KOOPA_RVT_INTEGER) {
            auto ret = std::to_string(ret_val->kind.data.integer.value);
            dump_riscv_inst("li", "a0", ret);
        } else if (ret_val->kind.tag == KOOPA_RVT_LOAD) {
            auto offset = runtime_stack.top().get_offset(ret_val);
            dump_lw("a0", offset);
        } else if (ret_val->kind.tag == KOOPA_RVT_CALL) {
            auto offset = runtime_stack.top().get_offset(ret_val);
            dump_lw("a0", offset);
        } else {
            auto tag = to_koopa_raw_value_tag(ret_val->kind.tag);
            std::cerr << "RETURN val type: " << tag << std::endl;
            assert(false);
        }
    }

    // epilogue
    // recover registers
    for (auto &it : runtime_stack.top().saved_registers) {
        auto reg = it.first;
        auto offset = it.second.offset;
        dump_lw(reg, offset);
    }

    // pop stack frame
    auto frame_length = runtime_stack.top().get_length();
    if (frame_length <= 2048)
        dump_riscv_inst("addi", "sp", "sp", std::to_string(frame_length));
    else {
        dump_riscv_inst("li", "t0", std::to_string(frame_length));
        dump_riscv_inst("add", "sp", "sp", "t0");
    }
    dump_riscv_inst("ret");
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_branch(koopa_raw_value_t value) {
    std::string reg = "t3";
    auto cond = value->kind.data.branch.cond;
    if (cond->kind.tag == KOOPA_RVT_INTEGER) {
        dump_riscv_inst("li", reg,
                        std::to_string(cond->kind.data.integer.value));
    } else if (cond->kind.tag == KOOPA_RVT_BINARY) {
        auto offset = runtime_stack.top().get_offset(cond);
        dump_lw(reg, offset);
    } else if (cond->kind.tag == KOOPA_RVT_LOAD) {
        auto offset = runtime_stack.top().get_offset(cond);
        dump_lw(reg, offset);
    } else if (cond->kind.tag == KOOPA_RVT_CALL) {
        auto offset = runtime_stack.top().get_offset(cond);
        dump_lw(reg, offset);
    } else {
        auto tag = to_koopa_raw_value_tag(cond->kind.tag);
        std::cerr << "branch val type: " << tag << std::endl;
        assert(false);
    }

    auto true_bb_name = value->kind.data.branch.true_bb->name + 1;
    auto false_bb_name = value->kind.data.branch.false_bb->name + 1;

    dump_riscv_inst("bnez", reg, true_bb_name);
    dump_riscv_inst("j", false_bb_name);

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_jump(koopa_raw_value_t value) {
    auto bb_name = value->kind.data.jump.target->name + 1;
    dump_riscv_inst("j", bb_name);

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_call(koopa_raw_value_t value) {
    auto args = value->kind.data.call.args;

    // load args to register and caller's stack frame
    assert(args.kind == KOOPA_RSIK_VALUE);
    for (int i = 0; i < args.len; i++) {
        koopa_raw_value_t val = (koopa_raw_value_t)args.buffer[i];
        if (i < 8) {
            auto reg = "a" + std::to_string(i);
            if (val->kind.tag == KOOPA_RVT_INTEGER) {
                auto int_val = val->kind.data.integer.value;
                dump_riscv_inst("li", reg, std::to_string(int_val));
            } else {
                // for simplicity, assume it's on the stack
                auto offset = runtime_stack.top().get_offset(val);
                dump_lw(reg, offset);
            }
        } else {
            auto reg = "t5";  // nobody use it for now
            if (val->kind.tag == KOOPA_RVT_INTEGER) {
                auto int_val = val->kind.data.integer.value;
                dump_riscv_inst("li", reg, std::to_string(int_val));
            } else {
                // for simplicity, assume it's on the stack
                auto offset = runtime_stack.top().get_offset(val);
                dump_lw(reg, offset);
            }
            dump_sw(reg, (i - 8) * 4);
        }
    }
    auto callee_name = value->kind.data.call.callee->name;
    dump_riscv_inst("call", callee_name + 1);

    // set return value if there is
    auto ret_type = value->ty->tag;
    if (ret_type == KOOPA_RTT_INT32) {
        auto offset = runtime_stack.top().get_offset(value);
        dump_sw("a0", offset);
    } else if (ret_type == KOOPA_RTT_UNIT) {
    } else {
        auto tag = to_koopa_raw_type_tag(ret_type);
        std::cerr << "Callee func return type: " << tag << std::endl;
        assert(false);
    }

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_global_alloc(
    koopa_raw_value_t value) {
    out << "  .data" << std::endl;
    out << "  .globl " << value->name + 1 << std::endl;
    out << value->name + 1 << ":" << std::endl;

    auto init = value->kind.data.global_alloc.init;
    auto init_type = init->kind.tag;
    if (init_type == KOOPA_RVT_ZERO_INIT) {
        out << "  .zero 4" << std::endl;
    } else if (init_type == KOOPA_RVT_INTEGER) {
        out << "  .word " << init->kind.data.integer.value << std::endl;
    } else {
        auto tag = to_koopa_raw_value_tag(init_type);
        std::cerr << "Global alloc: unknown tag " << tag << std::endl;
        assert(false);
    }

    out << std::endl;
    return 0;
}