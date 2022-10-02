#pragma once

#include <cassert>
#include <cstring>
#include <map>
#include <stack>
#include <utility>
#include <vector>

class SymbolTableEntry {
   public:
    bool is_const;
    int val;
};

typedef std::map<std::string, SymbolTableEntry> symbol_table_block_t;

class SymbolTable {
   private:
    std::vector<symbol_table_block_t> block_stack;

   public:
    bool exist_entry(std::string name);
    bool is_const_entry(std::string name);
    void insert_entry(std::string name, SymbolTableEntry entry);
    void get_entry_val(std::string name, int &val);
    void write_entry_val(std::string name, int val);
    void push_block();
    void pop_block();
};

typedef enum {
    BASIC_BLOCK_ENDING_STATUS_NULL,
    BASIC_BLOCK_ENDING_STATUS_BRANCH,
    BASIC_BLOCK_ENDING_STATUS_RETURN,
    BASIC_BLOCK_ENDING_STATUS_JUMP,
} basic_block_ending_status_t;

class BasicBlockInfo {
   public:
    basic_block_ending_status_t ending;
    std::string next;

    BasicBlockInfo() : ending(BASIC_BLOCK_ENDING_STATUS_NULL), next("") {}
};

class ControlFlow {
   private:
    std::map<std::string, BasicBlockInfo> cfg;
   public:
    std::string cur_block = "";
    bool to_next_block = false;

    void insert_info(std::string name, BasicBlockInfo info);
    void switch_control();
    basic_block_ending_status_t check_ending_status();
    void modify_ending_status(basic_block_ending_status_t status);
};

// Save information when generating koopa IR
class IRGenerator {
   private:
    int cnt_val;
    int cnt_block;

   public:
    IRGenerator() {
        cnt_val = 0;
        cnt_block = 0;
    }
    std::stack<std::string> stack_val;  // parse exp
    SymbolTable symbol_table;
    ControlFlow control_flow;

    std::string new_val() {
        auto val = cnt_val++;
        return "%" + std::to_string(val);
    }
    std::string new_block() {
        auto block = cnt_block++;
        return "%bb_" + std::to_string(block);
    }
};
