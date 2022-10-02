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
};

class StackFrame {
   private:
    std::map<koopa_raw_value_t, StackInfo> entries;
    int length = 0;

   public:
    bool is_returned = false;

    StackFrame(koopa_raw_slice_t bb_slice) {
        assert(bb_slice.kind == KOOPA_RSIK_BASIC_BLOCK);
        for (size_t i = 0; i < bb_slice.len; i++) {
            auto bb = (koopa_raw_basic_block_t)bb_slice.buffer[i];
            auto val_slice = bb->insts;
            assert(val_slice.kind == KOOPA_RSIK_VALUE);
            for (size_t j = 0; j < val_slice.len; j++) {
                auto val = (koopa_raw_value_t)val_slice.buffer[j];
                StackInfo stack_info;
                // check val type
                if (val->ty->tag == KOOPA_RTT_UNIT) continue;
                stack_info.offset = length;
                length += 4;
                entries.insert(std::make_pair(val, stack_info));
            }
        }
        // length align to 16
        length = ((length + 15) >> 4) << 4;
    }

    StackInfo get_info(koopa_raw_value_t val) {
        auto it = entries.find(val);
        if (it == entries.end()) {
            for (auto &it: entries)
                std:: cerr << it.first << std::endl;
            std::cerr << "val address: " << val << std::endl;
            std::cerr << "val type: " << val->kind.tag << std::endl;
            std::cerr << "val return type: " << val->ty->tag << std::endl;
            std::cerr << "No info on stack: " << val->name << std::endl;
            assert(false);
        }
        return it->second;
    }

    int get_length() { return length; }
};

class TargetCodeGenerator {
   public:
    koopa_raw_program_t raw;
    std::ostream &out;

    RegisterFile regfiles;
    std::stack<StackFrame> runtime_stack;

    TargetCodeGenerator(const char *koopa_file, std::ostream &out) : out{out} {
        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_file(koopa_file, &program);
        assert(ret == KOOPA_EC_SUCCESS);
        builder = koopa_new_raw_program_builder();
        raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);
    }
    ~TargetCodeGenerator() { koopa_delete_raw_program_builder(builder); }

    int dump_riscv();

   private:
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