#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <stack>
#include <utility>
#include <vector>
#include <set>

class SymbolTableEntry {
   public:
    bool is_const;
    int val;
    int alias; // suffix to differ local vars, added automatically
    bool is_func_param;  // use % instead of @
};

class FuncSymbolTableEntry {
    public:
    std::string func_type;
};

typedef std::map<std::string, SymbolTableEntry> symbol_table_block_t;

class SymbolTable {
   private:
    std::vector<symbol_table_block_t> block_stack;
    std::map<std::string, int> alias_cnt;
    std::map<std::string, FuncSymbolTableEntry> funcs;
    bool pending_push = false;

   public:
    bool exist_entry(std::string name);
    bool is_const_entry(std::string name);
    void insert_entry(std::string name, SymbolTableEntry entry);
    int get_entry_val(std::string name);
    void write_entry_val(std::string name, int val);
    void push_block(bool by_func_def=false);
    void pop_block();
    std::string get_aliased_name(std::string name, bool with_prefix=true);
    void insert_func_entry(std::string name, FuncSymbolTableEntry entry);
    std::string get_func_entry_type(std::string name);
};

typedef enum {
    BASIC_BLOCK_ENDING_STATUS_NULL,
    BASIC_BLOCK_ENDING_STATUS_BRANCH,
    BASIC_BLOCK_ENDING_STATUS_RETURN,
    BASIC_BLOCK_ENDING_STATUS_JUMP,      // the default fall-through
    BASIC_BLOCK_ENDING_STATUS_BREAK,     // jump caused by break
    BASIC_BLOCK_ENDING_STATUS_CONTINUE,  // jump caused by continue
    BASIC_BLOCK_ENDING_STATUS_UNREACHABLE,
} basic_block_ending_status_t;

class BasicBlockInfo {
   public:
    basic_block_ending_status_t ending;
    std::string dst_break;
    std::string dst_continue;
    std::vector<std::string> edge_in;
    std::vector<std::string> edge_out;

    BasicBlockInfo()
        : ending(BASIC_BLOCK_ENDING_STATUS_NULL),
          dst_break(""),
          dst_continue("") {}
};

class ControlFlow {
   private:
    std::map<std::string, BasicBlockInfo> cfg;

   public:
    std::string cur_block = "";

    void insert_info(std::string name, BasicBlockInfo info);
    basic_block_ending_status_t check_ending_status(std::string name = "");
    std::string get_dst_break(std::string name = "");
    std::string get_dst_continue(std::string name = "");
    void modify_ending_status(basic_block_ending_status_t status,
                              std::string name = "");
    void add_control_edge(std::string dst, std::string src = "");
    bool is_reachable(std::string name = "");
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
        symbol_table.push_block();  // global symbol table
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
