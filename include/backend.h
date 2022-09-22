#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

#include "koopa.h"

class KoopaRiscvBackend {
   public:
    koopa_raw_program_t raw;
    std::fstream out;

    KoopaRiscvBackend(const char *koopa_file, const char *riscv_file);
    ~KoopaRiscvBackend();

    int dump_riscv();

   private:
    koopa_raw_program_builder_t builder;
    int dump_koopa_raw_slice(const koopa_raw_slice_t slice);
    int dump_koopa_raw_value(const koopa_raw_value_t value);
    int dump_koopa_raw_function(const koopa_raw_function_t func);
};