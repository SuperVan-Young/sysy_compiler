#include <backend.h>

KoopaRiscvBackend::KoopaRiscvBackend(const char *koopa_file,
                                     const char *riscv_file) {
    // build koopa raw program
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_file(koopa_file, &program);
    assert(ret == KOOPA_EC_SUCCESS);
    builder = koopa_new_raw_program_builder();
    raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    // open output file
    out.open(riscv_file, std::ios::out);
    assert(out.is_open());

    // innitialize regfile
    tmp_regs.clear();
    for (int i = 0; i <= 7; i++) {
        std::string name = "a" + std::to_string(i);
        tmp_regs.push_back({name, nullptr});
    }
    for (int i = 0; i <= 6; i++) {
        std::string name = "t" + std::to_string(i);
        tmp_regs.push_back({name, nullptr});
    }
}

KoopaRiscvBackend::~KoopaRiscvBackend() {
    koopa_delete_raw_program_builder(builder);
    out.close();
}

int KoopaRiscvBackend::dump_riscv() {
    out << "  .text" << std::endl;
    out << "  .globl main" << std::endl;
    int ret = dump_koopa_raw_slice(raw.funcs);
    return ret;
}

/*******************************************************************************
 * Regfile Operations
 ******************************************************************************/

// Find name of register containing val
std::string KoopaRiscvBackend::find_tmp_reg(koopa_raw_value_t val) {
    for (auto &reg : tmp_regs)
        if (reg.val == val) return reg.name;
    return "";
}

std::string KoopaRiscvBackend::new_tmp_reg(koopa_raw_value_t val) {
    for (auto &reg : tmp_regs)
        if (reg.val == nullptr) return reg.name;
    return "";
}

void KoopaRiscvBackend::write_tmp_reg(koopa_raw_value_t val, std::string name) {
    for (auto &reg : tmp_regs)
        if (reg.name == name) {
            reg.val = val;
            return;
        }
    assert(false);
}

/*******************************************************************************
 * Dumping to Riscv
 ******************************************************************************/

void KoopaRiscvBackend::dump_riscv_inst(std::string inst,
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

/**
 * @brief A universal handler for koopa_raw_slice.
 * It checks kind and choose a proper handling function.
 */
int KoopaRiscvBackend::dump_koopa_raw_slice(koopa_raw_slice_t slice) {
    for (size_t i = 0; i < slice.len; i++) {
        const void *p = slice.buffer[i];
        int ret;
        if (slice.kind == KOOPA_RSIK_FUNCTION)
            ret = dump_koopa_raw_function((koopa_raw_function_t)p);
        else if (slice.kind == KOOPA_RSIK_VALUE)
            // we should only reference inst array.
            ret = dump_koopa_raw_value_inst((koopa_raw_value_t)p);
        else if (slice.kind == KOOPA_RSIK_BASIC_BLOCK)
            ret = dump_koopa_raw_basic_block((koopa_raw_basic_block_t)p);
        else
            ret = -1;  // TODO: add other dumpers
        assert(!ret);
    }
    return 0;
}

/**
 * @brief A universal handler for koopa_raw_value instruction.
 * It checks value.kind.tag and properly handles it.
 */
int KoopaRiscvBackend::dump_koopa_raw_value_inst(koopa_raw_value_t value) {
    auto tag = value->kind.tag;
    if (tag == KOOPA_RVT_BINARY) {
        auto op = value->kind.data.binary.op;
        std::string lhs, rhs;
        bool recycle_lhs = false, recycle_rhs = false;

        // find lhs operand, assign a register if necessary
        auto lhs_val = value->kind.data.binary.lhs;
        if (lhs_val->kind.tag == KOOPA_RVT_BINARY) {
            lhs = find_tmp_reg(lhs_val);
            assert(lhs != "");
        } else if (lhs_val->kind.tag == KOOPA_RVT_INTEGER) {
            // new reg, dump inst, write back
            lhs = new_tmp_reg(lhs_val);
            assert(lhs != "");
            dump_riscv_inst("li", lhs,
                            std::to_string(lhs_val->kind.data.integer.value));
            write_tmp_reg(lhs_val, lhs);
            recycle_lhs = true;
        }

        // find rhs operand
        // imm format is available for andi/ori/xori/addi
        auto rhs_val = value->kind.data.binary.rhs;
        bool rhs_imm;
        if (rhs_val->kind.tag == KOOPA_RVT_BINARY) {
            rhs = find_tmp_reg(rhs_val);
            assert(rhs != "");
            rhs_imm = false;
        } else if (rhs_val->kind.tag == KOOPA_RVT_INTEGER) {
            if (op == KOOPA_RBO_AND || op == KOOPA_RBO_OR ||
                op == KOOPA_RBO_XOR || op == KOOPA_RBO_ADD) {
                rhs = std::to_string(rhs_val->kind.data.integer.value);
                rhs_imm = true;
            } else {
                // new reg, dump inst, write back
                rhs = new_tmp_reg(rhs_val);
                assert(rhs != "");
                dump_riscv_inst("li", rhs,
                            std::to_string(rhs_val->kind.data.integer.value));
                write_tmp_reg(rhs_val, rhs);
                rhs_imm = false;
                recycle_rhs = true;
            }
        }

        // given op type, dump the value
        auto reg = new_tmp_reg(value);
        assert(reg != "");

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

        // write the result back to regfile
        write_tmp_reg(value, reg);

        // recycle imm's registers
        if (recycle_lhs) write_tmp_reg(nullptr, lhs);
        if (recycle_rhs) write_tmp_reg(nullptr, rhs);

        return 0;

    } else if (tag == KOOPA_RVT_RETURN) {
        // move operand to a0
        auto ret_val = value->kind.data.ret.value;
        if (ret_val->kind.tag == KOOPA_RVT_BINARY) {
            auto ret = find_tmp_reg(ret_val);
            if (ret != "a0") dump_riscv_inst("mv", "a0", ret);
        } else if (ret_val->kind.tag == KOOPA_RVT_INTEGER) {
            auto ret = std::to_string(ret_val->kind.data.integer.value);
            dump_riscv_inst("li", "a0", ret);
        }
        // write the result back to regfile
        write_tmp_reg(value, "a0");
        dump_riscv_inst("ret");
        return 0;
    } else {
        std::cerr << "invalid value type." << std::endl;
        return -1;
    }
}

int KoopaRiscvBackend::dump_koopa_raw_function(koopa_raw_function_t func) {
    out << func->name + 1 << ":" << std::endl;
    int ret = dump_koopa_raw_slice(func->bbs);
    return ret;
}

int KoopaRiscvBackend::dump_koopa_raw_basic_block(koopa_raw_basic_block_t bb) {
    int ret = dump_koopa_raw_slice(bb->insts);
    return ret;
}