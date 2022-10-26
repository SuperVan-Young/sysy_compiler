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

// helper functions

int get_koopa_raw_value_size(koopa_raw_type_t ty) {
    if (ty->tag == KOOPA_RTT_INT32) {
        return 4;
    } else if (ty->tag == KOOPA_RTT_UNIT) {
        return 0;
    } else if (ty->tag == KOOPA_RTT_ARRAY) {
        int base = get_koopa_raw_value_size(ty->data.array.base);
        int len = ty->data.array.len;
        return base * len;
    } else if (ty->tag == KOOPA_RTT_POINTER) {
        return 4;
    } else if (ty->tag == KOOPA_RTT_FUNCTION) {
        assert(false);
    } else {
        assert(false);
    }
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

// load symbol / integer value to a given register
// This operation is not necessarily successful,
// caller should handle the exceptions
bool TargetCodeGenerator::load_value_to_reg(koopa_raw_value_t value,
                                            std::string reg) {
    if (value->kind.tag == KOOPA_RVT_INTEGER) {
        // integer
        auto int_val = std::to_string(value->kind.data.integer.value);
        dump_riscv_inst("li", reg, int_val);
    } else if (value->kind.tag == KOOPA_RVT_ALLOC ||
               value->kind.tag == KOOPA_RVT_LOAD ||
               value->kind.tag == KOOPA_RVT_GET_ELEM_PTR ||
               value->kind.tag == KOOPA_RVT_GET_PTR ||
               value->kind.tag == KOOPA_RVT_BINARY ||
               value->kind.tag == KOOPA_RVT_CALL) {
        auto offset = runtime_stack.top().get_koopa_value(value).offset;
        dump_lw(reg, offset);
    } else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        dump_riscv_inst("la", reg, value->name + 1);
    } else {
        return false;
    }
    return true;
}

void TargetCodeGenerator::dump_alloc_initializer(koopa_raw_value_t init,
                                                 int offset) {
    // std::cerr << "dump alloc initializer: " << offset << std::endl;

    // TODO: optimize dump sw
    auto init_type = init->kind.tag;
    if (init_type == KOOPA_RVT_ZERO_INIT) {
        int init_size = get_koopa_raw_value_size(init->ty);
        assert(init_size > 0);
        for (int i = 0; i < init_size; i += 4) {
            dump_sw("zero", offset + i);
        }
    } else if (init_type == KOOPA_RVT_INTEGER) {
        auto int_val = init->kind.data.integer.value;
        if (int_val == 0) {
            dump_sw("zero", offset);
        } else {
            dump_riscv_inst("li", "t0", std::to_string(int_val));
            dump_sw("t0", offset);
        }
    } else if (init_type == KOOPA_RVT_AGGREGATE) {
        auto elems = init->kind.data.aggregate.elems;
        for (int i = 0; i < elems.len; i++) {
            auto sub_init = (koopa_raw_value_t)elems.buffer[i];
            auto sub_init_size = get_koopa_raw_value_size(sub_init->ty);
            dump_alloc_initializer(sub_init, offset + i * sub_init_size);
        }
    } else {
        auto tag = to_koopa_raw_value_tag(init_type);
        std::cerr << "Dump initializer: unknown tag " << tag << std::endl;
        assert(false);
    }
}

// dump initializer recursively
void TargetCodeGenerator::dump_global_alloc_initializer(
    koopa_raw_value_t init) {
    auto init_type = init->kind.tag;
    if (init_type == KOOPA_RVT_ZERO_INIT) {
        int init_size = get_koopa_raw_value_size(init->ty);
        assert(init_size > 0);
        out << "  .zero " << init_size << std::endl;
    } else if (init_type == KOOPA_RVT_INTEGER) {
        out << "  .word " << init->kind.data.integer.value << std::endl;
    } else if (init_type == KOOPA_RVT_AGGREGATE) {
        auto elems = init->kind.data.aggregate.elems;
        for (int i = 0; i < elems.len; i++) {
            auto sub_init = (koopa_raw_value_t)elems.buffer[i];
            dump_global_alloc_initializer(sub_init);
        }
    } else {
        auto tag = to_koopa_raw_value_tag(init_type);
        std::cerr << "Dump initializer: unknown tag " << tag << std::endl;
        assert(false);
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
        return dump_koopa_raw_value_alloc(value);
    else if (tag == KOOPA_RVT_GLOBAL_ALLOC)
        return dump_koopa_raw_value_global_alloc(value);
    else if (tag == KOOPA_RVT_LOAD)
        return dump_koopa_raw_value_load(value);
    else if (tag == KOOPA_RVT_STORE)
        return dump_koopa_raw_value_store(value);
    else if (tag == KOOPA_RVT_GET_ELEM_PTR)
        return dump_koopa_raw_value_get_elem_ptr(value);
    else if (tag == KOOPA_RVT_GET_PTR)
        return dump_koopa_raw_value_get_ptr(value);
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
    std::string lhs = "t1", rhs = "t2";

    auto lhs_val = value->kind.data.binary.lhs;
    assert(load_value_to_reg(lhs_val, lhs));
    auto rhs_val = value->kind.data.binary.rhs;
    assert(load_value_to_reg(rhs_val, rhs));

    // TODO: use i instr to simplify!

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
        dump_riscv_inst(inst, reg, lhs, rhs);
    } else if (op == KOOPA_RBO_OR) {
        std::string inst = "or";
        dump_riscv_inst(inst, reg, lhs, rhs);
    } else if (op == KOOPA_RBO_XOR) {
        std::string inst = "xor";
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
    auto offset = runtime_stack.top().get_koopa_value(value).offset;
    dump_sw(reg, offset);

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_load(koopa_raw_value_t value) {
    // dereference the pointer
    std::string reg = "t0";

    auto src = value->kind.data.load.src;
    assert(load_value_to_reg(src, reg));

    dump_riscv_inst("lw", reg, "0(" + reg + ")");

    // record value result onto stack
    auto val_offset = runtime_stack.top().get_koopa_value(value).offset;
    dump_sw(reg, val_offset);
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_store(koopa_raw_value_t value) {
    auto src = value->kind.data.store.value;
    auto dst = value->kind.data.store.dest;

    assert(dst->ty->tag == KOOPA_RTT_POINTER);
    auto dst_base_type = dst->ty->data.pointer.base;

    if (dst_base_type->tag == KOOPA_RTT_ARRAY) {
        // local array initialization
        auto offset = runtime_stack.top().get_alloc_memory(dst).offset;
        dump_alloc_initializer(src, offset);

    } else if (dst_base_type->tag == KOOPA_RTT_INT32 ||
               dst_base_type->tag == KOOPA_RTT_POINTER) {
        std::string src_reg = "t0";  // what need to be stored
        if (src->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
            auto index = src->kind.data.func_arg_ref.index;
            if (index < 8) {
                src_reg = "a" + std::to_string(index);
            } else {
                // assume s0 has been set up
                int offset = (index - 8) * 4;
                dump_lw(src_reg, offset, "s0");
            }
        } else if (src->kind.tag == KOOPA_RVT_ZERO_INIT) {
            dump_riscv_inst("li", src_reg, std::to_string(0));
        } else {
            assert(load_value_to_reg(src, src_reg));
        }

        // load dst address
        std::string dst_reg = "t6";
        assert(load_value_to_reg(dst, dst_reg));

        dump_riscv_inst("sw", src_reg, "0(" + dst_reg + ")");

    } else {
        std::cerr << "Store: invalid dst base type" << std::endl;
        assert(false);
    }
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_return(koopa_raw_value_t value) {
    // move operand to a0
    auto ret_val = value->kind.data.ret.value;
    if (ret_val != nullptr) {
        assert(load_value_to_reg(ret_val, "a0"));
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
    std::string reg = "t0";
    auto cond = value->kind.data.branch.cond;
    assert(load_value_to_reg(cond, reg));

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
            load_value_to_reg(val, reg);
        } else {
            auto reg = "t0";  // nobody use it for now
            load_value_to_reg(val, reg);
            dump_sw(reg, (i - 8) * 4);
        }
    }
    auto callee_name = value->kind.data.call.callee->name;
    dump_riscv_inst("call", callee_name + 1);

    // set return value if there is
    auto ret_type = value->ty->tag;
    if (ret_type == KOOPA_RTT_INT32) {
        auto offset = runtime_stack.top().get_koopa_value(value).offset;
        dump_sw("a0", offset);
    } else if (ret_type == KOOPA_RTT_UNIT) {
    } else {
        auto tag = to_koopa_raw_type_tag(ret_type);
        std::cerr << "Callee func return type: " << tag << std::endl;
        assert(false);
    }

    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_alloc(koopa_raw_value_t value) {
    // return the alloced address
    std::string tmp_reg = "t0";

    auto alloc_info = runtime_stack.top().get_alloc_memory(value);
    if (alloc_info.offset <= 2047) {
        dump_riscv_inst("addi", tmp_reg, "sp",
                        std::to_string(alloc_info.offset));
    } else {
        dump_riscv_inst("li", tmp_reg, std::to_string(alloc_info.offset));
        dump_riscv_inst("add", tmp_reg, tmp_reg, "sp");
    }

    auto val_offset = runtime_stack.top().get_koopa_value(value).offset;
    dump_sw(tmp_reg, val_offset);
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_global_alloc(
    koopa_raw_value_t value) {
    out << "  .data" << std::endl;
    out << "  .globl " << value->name + 1 << std::endl;
    out << value->name + 1 << ":" << std::endl;

    auto init = value->kind.data.global_alloc.init;
    dump_global_alloc_initializer(init);

    out << std::endl;
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_get_elem_ptr(
    koopa_raw_value_t value) {
    std::string base_reg = "t0";
    std::string index_reg = "t1";
    std::string elem_size_reg = "t2";

    // load base ptr address
    auto src = value->kind.data.get_ptr.src;
    load_value_to_reg(src, base_reg);

    // calculate index
    auto index = value->kind.data.get_elem_ptr.index;
    assert(load_value_to_reg(index, index_reg));

    // elem size
    assert(src->ty->tag == KOOPA_RTT_POINTER);
    auto ptr_base_ty = src->ty->data.pointer.base;
    assert(ptr_base_ty->tag == KOOPA_RTT_ARRAY);
    auto elem_ty = ptr_base_ty->data.array.base;
    int elem_size = get_koopa_raw_value_size(elem_ty);
    dump_riscv_inst("li", elem_size_reg, std::to_string(elem_size));

    dump_riscv_inst("mul", index_reg, index_reg, elem_size_reg);
    dump_riscv_inst("add", base_reg, base_reg, index_reg);

    // record value result
    auto val_offset = runtime_stack.top().get_koopa_value(value).offset;
    dump_sw(base_reg, val_offset);
    return 0;
}

int TargetCodeGenerator::dump_koopa_raw_value_get_ptr(koopa_raw_value_t value) {
    std::string base_reg = "t0";
    std::string index_reg = "t1";
    std::string elem_size_reg = "t2";

    // load base ptr address
    auto src = value->kind.data.get_ptr.src;
    load_value_to_reg(src, base_reg);

    // calculate index
    auto index = value->kind.data.get_ptr.index;
    assert(load_value_to_reg(index, index_reg));

    // base ptr's base (...) size
    assert(src->ty->tag == KOOPA_RTT_POINTER);
    auto ptr_base_ty = src->ty->data.pointer.base;
    int elem_size = get_koopa_raw_value_size(ptr_base_ty);
    dump_riscv_inst("li", elem_size_reg, std::to_string(elem_size));

    dump_riscv_inst("mul", index_reg, index_reg, elem_size_reg);
    dump_riscv_inst("add", base_reg, base_reg, index_reg);

    auto val_offset = runtime_stack.top().get_koopa_value(value).offset;
    dump_sw(base_reg, val_offset);
    return 0;
}
