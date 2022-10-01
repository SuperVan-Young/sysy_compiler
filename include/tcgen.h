#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stack>
#include <utility>
#include <vector>

#include "koopa.h"

class RegisterInfo {
   public:
    koopa_raw_value_t val;
    RegisterInfo() : val(nullptr) {}
};

class RegisterFile {
   private:
    std::map<std::string, RegisterInfo> user_regs;

   public:
    RegisterFile() {
        // innitialize user regs
        for (int i = 0; i <= 7; i++) {
            std::string name = "a" + std::to_string(i);
            user_regs.insert(std::make_pair(name, RegisterInfo()));
        }
        for (int i = 0; i <= 6; i++) {
            std::string name = "t" + std::to_string(i);
            user_regs.insert(std::make_pair(name, RegisterInfo()));
        }
    }

    bool exist_value(koopa_raw_value_t val) {
        for (auto &e : user_regs)
            if (e.second.val == val) return true;
        return false;
    }

    std::string get_reg(koopa_raw_value_t val) {
        for (auto &e : user_regs)
            if (e.second.val == val) return e.first;
        assert(false);
    }

    koopa_raw_value_t read_value(std::string reg) {
        for (auto &e: user_regs) 
            if (e.first == reg) return e.second.val;
        return nullptr;
    }

    void write_value(std::string reg, koopa_raw_value_t val) {
        auto it = user_regs.find(reg);
        assert(it != user_regs.end());
        it->second.val = val;
    }
};

class StackFrame {};

class TargetCodeGenerator {
   public:
    koopa_raw_program_t raw;
    std::fstream out;

    RegisterFile regfiles;
    std::stack<StackFrame> runtime_stack;

    TargetCodeGenerator(const char *koopa_file, std::ostream &out);
    ~TargetCodeGenerator();

    int dump_riscv();

   private:
    koopa_raw_program_builder_t builder;

    // dumping to riscv
    void dump_riscv_inst(std::string inst, std::string reg_0, std::string reg_1,
                         std::string reg_2);

    int dump_koopa_raw_slice(koopa_raw_slice_t slice);
    int dump_koopa_raw_value_inst(koopa_raw_value_t value);
    int dump_koopa_raw_function(koopa_raw_function_t func);
    int dump_koopa_raw_basic_block(koopa_raw_basic_block_t bb);
};