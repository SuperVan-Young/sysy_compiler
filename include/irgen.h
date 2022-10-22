#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <utility>
#include <vector>

typedef enum {
    SYMBOL_TABLE_ENTRY_VAR,
    SYMBOL_TABLE_ENTRY_FUNC,
    SYMBOL_TABLE_ENTRY_ARRAY,
} symbol_table_entry_type_t;

class SymbolTableEntry {
   public:
    symbol_table_entry_type_t type;
    // var & array
    bool is_named;  // prefix, @ if true, % otherwise
    int alias;      // suffix to differ local vars, added automatically
    // var
    bool is_const;  // const var
    int val;        // init value of const var
    // func
    std::string func_type;
    // array
    std::vector<int> array_size;
};

typedef std::map<std::string, SymbolTableEntry> symbol_table_block_t;

class SymbolTable {
   private:
    symbol_table_block_t global_table;
    std::vector<symbol_table_block_t> block_stack;  // local
    std::map<std::string, int> alias_cnt;

    int _get_alias(std::string name);
    bool _get_local_table(symbol_table_block_t *&table);
    bool _get_entry(std::string name, SymbolTableEntry *&entry);

   public:
    // insert new entry
    void insert_var_entry(std::string name);
    void insert_const_var_entry(std::string name, int val);
    void insert_func_entry(std::string name, std::string func_type);
    void insert_array_entry(std::string name, std::vector<int> array_size);

    // get entry info
    bool is_global_var_entry(std::string name);
    bool is_const_var_entry(std::string name);
    int get_const_var_val(std::string name);
    std::string get_var_name(std::string name);
    std::string get_func_entry_type(std::string name);

    // basic block stacking
    void push_block();
    void pop_block();
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
    std::string dst_jump;      // default fall-through target
    std::string dst_break;     // break target
    std::string dst_continue;  // continue target
    std::vector<std::string> edge_in;
    std::vector<std::string> edge_out;

    BasicBlockInfo() : ending(BASIC_BLOCK_ENDING_STATUS_NULL) {}
    BasicBlockInfo(std::string dst_jump)
        : ending(BASIC_BLOCK_ENDING_STATUS_NULL), dst_jump(dst_jump) {}
    BasicBlockInfo(std::string dst_jump, std::string dst_break,
                   std::string dst_continue)
        : ending(BASIC_BLOCK_ENDING_STATUS_NULL),
          dst_jump(dst_jump),
          dst_break(dst_break),
          dst_continue(dst_continue) {}
};

class ControlFlow {
   private:
    std::map<std::string, BasicBlockInfo> cfg;

   public:
    std::string cur_block = "";

    // inserting new blocks
    void insert_if(std::string name_then, std::string name_end);
    void insert_if_else(std::string name_then, std::string name_else,
                        std::string name_end);
    void insert_while(std::string name_entry, std::string name_body,
                      std::string name_end);

    // switching control flow
    void init_entry_block(std::string name, std::ostream &out);
    bool switch_control_flow(std::string name, std::ostream &out);
    void _break(std::ostream &out);
    void _continue(std::ostream &out);

    basic_block_ending_status_t check_ending_status();
    void modify_ending_status(basic_block_ending_status_t status);
    void add_control_edge(std::string dst, std::string src = "");
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
