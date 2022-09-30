#pragma once

#include <cstring>
#include <map>
#include <stack>
#include <cassert>
#include <utility>

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

    std::string new_val() {
        auto val = cnt_val++;
        return "%" + std::to_string(val);
    }
};

class SymbolTableEntry {
   public:
    bool has_val;
    int val;
};

class SymbolTable {
   private:
    std::map<std::string, SymbolTableEntry> entries;
   public:
    bool exist_entry(std::string name) {
        auto iter = entries.find(name);
        return iter == entries.end();
    }

    bool insert_entry(std::string name, SymbolTableEntry entry) {
        entries.insert(std::pair<std::string, SymbolTableEntry>(name, entry));
    }

    bool get_entry_val(std::string name) {
        auto &entry = entries[name];
        assert(entry.has_val);
        return entry.val;
    }
};