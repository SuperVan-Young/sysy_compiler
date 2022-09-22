#include <backend.h>

KoopaRiscvBackend::KoopaRiscvBackend(const char *koopa_file, const char *riscv_file) {
    // build koopa raw program
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_file(koopa_file, &program);
    assert(ret == KOOPA_EC_SUCCESS);
    builder = koopa_new_raw_program_builder();
    raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    // open output file
    std::fstream out;
    out.open(riscv_file, std::ios::out);
    assert(out.is_open());
}

KoopaRiscvBackend::~KoopaRiscvBackend() {
    koopa_delete_raw_program_builder(builder);
    out.close();
}

int KoopaRiscvBackend::dump_riscv() {
    out << "  .text" << std::endl;
    out << "  .globl main" << std::endl;
    dump_koopa_raw_slice(raw.funcs);
}

/**
 * @brief A universal handler for koopa_raw_slice.
 * It checks kind and choose a proper handling function.
 * 
 * @param slice 
 * @return int 
 */
int KoopaRiscvBackend::dump_koopa_raw_slice(const koopa_raw_slice_t slice) {
    for (size_t i = 0; i < slice.len; i++) {
        const void *p = slice.buffer[i];
        int ret;
        if (slice.kind == KOOPA_RSIK_FUNCTION)
            ret = dump_koopa_raw_function((const koopa_raw_function_t) p);
        else if (slice.kind == KOOPA_RSIK_VALUE)
            ret = dump_koopa_raw_value((const koopa_raw_value_t) p);
        else if (slice.kind == KOOPA_RSIK_BASIC_BLOCK)
            ret = dump_koopa_raw_basic_block((const koopa_raw_basic_block_t) p);
        else
            ret = -1; // TODO: add other dumpers
        assert(!ret);
    }
    return 0;
}

/**
 * @brief A universal handler for koopa_raw_value instrs.
 * It checks value.kind.tag and choose a proper handling function.
 * TODO: (We should only dump inst. Here for lv2, we ignore many checkings)
 * 
 * @param value 
 * @return int 
 */
int KoopaRiscvBackend::dump_koopa_raw_value(const koopa_raw_value_t value) {
    int ret;
    auto tag = value->kind.tag;
    if (tag == KOOPA_RVT_RETURN)  {
        auto ret_value = value->kind.data.ret.value;
        assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
        int32_t int_val = ret_value->kind.data.integer.value;
        std::cerr << int_val;

        out << "li a0, 0" << std::endl;
        out << "ret" << std::endl;
        ret = 0;
    } else {
        ret = -1;
    }
    return ret;
}

int KoopaRiscvBackend::dump_koopa_raw_function(const koopa_raw_function_t func) {
    out << func->name << ":" << std::endl;
    int ret = dump_koopa_raw_slice(func->bbs);
    return ret;
}

int KoopaRiscvBackend::dump_koopa_raw_basic_block(const koopa_raw_basic_block_t bb) {
    int ret = dump_koopa_raw_slice(bb->insts);
    return ret;
}