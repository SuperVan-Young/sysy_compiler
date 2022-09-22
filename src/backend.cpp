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
    dump_koopa_raw_slice(raw.values);
    dump_koopa_raw_slice(raw.funcs);
}

int KoopaRiscvBackend::dump_koopa_raw_slice(const koopa_raw_slice_t slice) {
    for (size_t i = 0; i < slice.len; i++) {
        const void *p = slice.buffer[i];
        int ret;
        if (slice.kind == KOOPA_RSIK_FUNCTION)
            ret = dump_koopa_raw_function((const koopa_raw_function_t) p);
        else if (slice.kind == KOOPA_RSIK_FUNCTION)
            ret = dump_koopa_raw_value((const koopa_raw_value_t) p);
        else
            ret = -1; // TODO: add other dumpers
        assert(!ret);
    }
    return 0;
}
