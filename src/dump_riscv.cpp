#include <tcgen.h>

int TargetCodeGenerator::dump_riscv() {
    out << "  .text" << std::endl;
    out << "  .globl main" << std::endl;
    int ret = dump_koopa_raw_slice(raw.funcs);
    return ret;
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

void TargetCodeGenerator::dump_lw(std::string reg, int offset) {
    if (offset <= 2047 && offset >= -2048) {
        dump_riscv_inst("lw", reg, std::to_string(offset) + "(sp)");
    } else {
        // take an empty register for offset
        // TODO: before considering register allocation, we use t6
        dump_riscv_inst("li", "t6", std::to_string(offset));
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
    // function name, ignore first character
    out << func->name + 1 << ":" << std::endl;

    runtime_stack.push(StackFrame(func->bbs));
    // prologue insts
    int frame_length = runtime_stack.top().get_length();
    if (frame_length <= 2048)
        dump_riscv_inst("addi", "sp", "sp", std::to_string(-frame_length));
    else {
        dump_riscv_inst("li", "t0", std::to_string(-frame_length));
        dump_riscv_inst("add", "sp", "sp", "t0");
        // length will never be used, so everyone can use t0 later
    }

    int ret = dump_koopa_raw_slice(func->bbs);

    // for function without a return inst, we add a default one.
    if (!runtime_stack.top().is_returned) {
        // epilogue insts
        if (frame_length <= 2048)
            dump_riscv_inst("addi", "sp", "sp", std::to_string(frame_length));
        else {
            dump_riscv_inst("li", "t0", std::to_string(frame_length));
            dump_riscv_inst("add", "sp", "sp", "t0");
        }
        runtime_stack.top().is_returned = true;
        dump_riscv_inst("ret");
    }
    runtime_stack.pop();

    return ret;
}

int TargetCodeGenerator::dump_koopa_raw_basic_block(
    koopa_raw_basic_block_t bb) {
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
    else if (tag == KOOPA_RVT_LOAD)
        return dump_koopa_raw_value_load(value);
    else if (tag == KOOPA_RVT_STORE)
        return dump_koopa_raw_value_store(value);
    else {
        std::cerr << "Raw value type " << tag << " not implemented."
                  << std::endl;
        return -1;
    }
}

int TargetCodeGenerator::dump_koopa_raw_value_binary(koopa_raw_value_t value) {
    auto op = value->kind.data.binary.op;
    std::string lhs, rhs;
    // t0 = t1 op t2

    // find lhs operand, assign a register if necessary
    auto lhs_val = value->kind.data.binary.lhs;
    if (lhs_val->kind.tag == KOOPA_RVT_BINARY) {
        // binary: load from stack
        auto offset = runtime_stack.top().get_info(lhs_val).offset;
        dump_lw("t1", offset);
        lhs = "t1";
    } else if (lhs_val->kind.tag == KOOPA_RVT_INTEGER) {
        auto lhs_int = lhs_val->kind.data.integer.value;
        dump_riscv_inst("li", "t1", std::to_string(lhs_int));
        lhs = "t1";
    } else if (lhs_val->kind.tag == KOOPA_RVT_LOAD) {
        auto offset = runtime_stack.top().get_info(lhs_val).offset;
        dump_lw("t1", offset);
        lhs = "t1";
    } else {
        std::cerr << "BINARY val type: " << lhs_val->kind.tag << std::endl;
        assert(false);
    }

    // find rhs operand
    // imm format is available for andi/ori/xori/addi
    auto rhs_val = value->kind.data.binary.rhs;
    bool rhs_imm = false;  // TODO: simplify rhs imm
    if (rhs_val->kind.tag == KOOPA_RVT_BINARY) {
        // binary: load from stack
        auto offset = runtime_stack.top().get_info(rhs_val).offset;
        dump_lw("t2", offset);
        rhs = "t2";
    } else if (rhs_val->kind.tag == KOOPA_RVT_INTEGER) {
        auto rhs_int = rhs_val->kind.data.integer.value;
        dump_riscv_inst("li", "t2", std::to_string(rhs_int));
        rhs = "t2";
    } else if (rhs_val->kind.tag == KOOPA_RVT_LOAD) {
        auto offset = runtime_stack.top().get_info(rhs_val).offset;
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
    auto offset = runtime_stack.top().get_info(value).offset;
    dump_sw(reg, offset);

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_load(koopa_raw_value_t value) {
    // store the return value back on stack, kind of stupid...
    auto src = value->kind.data.load.src;
    auto src_offset = runtime_stack.top().get_info(src).offset;
    dump_lw("t0", src_offset);
    auto dst_offset = runtime_stack.top().get_info(value).offset;
    dump_sw("t0", dst_offset);
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_store(koopa_raw_value_t value) {
    // load value from stack, and put back to dest ...
    auto src = value->kind.data.store.value;
    if (src->kind.tag == KOOPA_RVT_INTEGER) {
        int src_val = src->kind.data.integer.value;
        dump_riscv_inst("li", "t0", std::to_string(src_val));
    } else {
        std::cerr << "STORE src type: " << src->kind.tag << std::endl;
        assert(false);
    }
    auto dst = value->kind.data.store.dest;
    auto dst_offset = runtime_stack.top().get_info(dst).offset;
    dump_sw("t0", dst_offset);
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_return(koopa_raw_value_t value) {
    // move operand to a0
    auto ret_val = value->kind.data.ret.value;
    if (ret_val->kind.tag == KOOPA_RVT_BINARY) {
        auto offset = runtime_stack.top().get_info(ret_val).offset;
        dump_lw("a0", offset);
    } else if (ret_val->kind.tag == KOOPA_RVT_INTEGER) {
        auto ret = std::to_string(ret_val->kind.data.integer.value);
        dump_riscv_inst("li", "a0", ret);
    } else if (ret_val->kind.tag == KOOPA_RVT_LOAD) {
        auto offset = runtime_stack.top().get_info(ret_val).offset;
        dump_lw("a0", offset);
    } else {
        std::cerr << "RETURN val type: " << ret_val->kind.tag << std::endl;
        assert(false);
    }

    // epilogue
    auto frame_length = runtime_stack.top().get_length();
    if (frame_length <= 2048)
        dump_riscv_inst("addi", "sp", "sp", std::to_string(frame_length));
    else {
        dump_riscv_inst("li", "t0", std::to_string(frame_length));
        dump_riscv_inst("add", "sp", "sp", "t0");
    }
    dump_riscv_inst("ret");
    runtime_stack.top().is_returned = true;
    return 0;
}