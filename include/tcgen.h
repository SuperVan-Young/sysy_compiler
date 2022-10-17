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
        for (auto &e : user_regs)
            if (e.first == reg) return e.second.val;
        return nullptr;
    }

    void write_value(std::string reg, koopa_raw_value_t val) {
        auto it = user_regs.find(reg);
        assert(it != user_regs.end());
        it->second.val = val;
    }
};

class StackInfo {
   public:
    int offset;
    int size = 4;  // default size

    StackInfo() {}
    StackInfo(int offset_) : offset(offset_) {}
};

class StackFrame {
   private:
    std::map<std::string, StackInfo> saved_registers;
    std::map<koopa_raw_value_t, StackInfo> entries;
    int length = 0;

   public:
    bool is_returned = false;

    StackFrame(koopa_raw_function_t func);
    int get_offset(koopa_raw_value_t val);
    int get_length() { return length; }
    void recover_registers();
};

class TargetCodeGenerator {
   public:
    TargetCodeGenerator(const char *koopa_file, std::ostream &out);
    ~TargetCodeGenerator();

    int dump_riscv();

   private:
    RegisterFile regfiles;
    std::stack<StackFrame> runtime_stack;

    koopa_raw_program_t raw;
    std::ostream &out;
    koopa_raw_program_builder_t builder;

    void dump_riscv_inst(std::string inst, std::string reg_0, std::string reg_1,
                         std::string reg_2);
    void dump_lw(std::string reg, int offset);
    void dump_sw(std::string reg, int offset);

    int dump_koopa_raw_slice(koopa_raw_slice_t slice);
    int dump_koopa_raw_function(koopa_raw_function_t func);
    int dump_koopa_raw_basic_block(koopa_raw_basic_block_t bb);
    int dump_koopa_raw_value(koopa_raw_value_t value);

    int dump_koopa_raw_value_binary(koopa_raw_value_t value);
    int dump_koopa_raw_value_load(koopa_raw_value_t value);
    int dump_koopa_raw_value_store(koopa_raw_value_t value);
    int dump_koopa_raw_value_return(koopa_raw_value_t value);
    int dump_koopa_raw_value_branch(koopa_raw_value_t value);
    int dump_koopa_raw_value_jump(koopa_raw_value_t value);
};