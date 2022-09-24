#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#include "koopa.h"

typedef struct {
    std::string name;
    koopa_raw_value_t val;
} riscv_register_t;

class KoopaRiscvBackend {
   public:
    koopa_raw_program_t raw;
    std::fstream out;

    KoopaRiscvBackend(const char *koopa_file, const char *riscv_file);
    ~KoopaRiscvBackend();

    int dump_riscv();

   private:
    koopa_raw_program_builder_t builder;
    std::vector<riscv_register_t> tmp_regs;

    // regfile operations

    std::string find_tmp_reg(koopa_raw_value_t val);
    std::string new_tmp_reg(koopa_raw_value_t val);
    void write_tmp_reg(koopa_raw_value_t val, std::string name);

    // dumping to riscv
    void dump_riscv_inst(std::string inst, std::string reg_0,
                                std::string reg_1, std::string reg_2);

    int dump_koopa_raw_slice(koopa_raw_slice_t slice);
    int dump_koopa_raw_value_inst(koopa_raw_value_t value);
    int dump_koopa_raw_function(koopa_raw_function_t func);
    int dump_koopa_raw_basic_block(koopa_raw_basic_block_t bb);
};