#pragma once

#include <cstring>
#include <map>
#include <stack>
#include <cassert>
#include <utility>

class SymbolTableEntry {
   public:
    bool is_const;
    bool has_val;
    int val;
};

class SymbolTable {
   private:
    std::map<std::string, SymbolTableEntry> entries;
   public:
    bool exist_entry(std::string name) {
        auto iter = entries.find(name);
        return iter != entries.end();
    }

    void insert_entry(std::string name, SymbolTableEntry entry) {
        assert(!exist_entry(name));
        entries.insert(std::pair<std::string, SymbolTableEntry>(name, entry));
    }

    bool get_entry_val(std::string name, int &val) {
        auto &entry = entries[name];
        val = entry.val;
        return entry.has_val;
    }

    bool is_const_entry(std::string name) {
        auto iter = entries.find(name);
        return iter->second.is_const;
    }

    void write_entry_val(std::string name, int val) {
        auto &entry = entries[name];
        entry.val = val;
        entry.has_val = true;
    }
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

    std::string new_val() {
        auto val = cnt_val++;
        return "%" + std::to_string(val);
    }
    std::string new_block() {
        auto block = cnt_block++;
        return "%bb_" + std::to_string(block);
    }
};
