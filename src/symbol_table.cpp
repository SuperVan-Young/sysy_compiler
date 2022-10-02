#include "irgen.h"

// recursively check if a symbol exists
bool SymbolTable::exist_entry(std::string name) {
    // recursive searching
    for (auto it_b = block_stack.rbegin(); it_b!= block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) return true;
    }
    return false;
}

// recursively check if a symbol is const
bool SymbolTable::is_const_entry(std::string name) {
    // recursive searching
    for (auto it_b = block_stack.rbegin(); it_b!= block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) return it_entry->second.is_const;
    }
    assert(false);
}

// insert a new entry to current block's symbol table
void SymbolTable::insert_entry(std::string name, SymbolTableEntry entry) {
    // current block shouldn't have this entry
    auto it_b = block_stack.rbegin();
    assert(it_b != block_stack.rend());
    assert(it_b->find(name) == it_b->end());

    it_b->insert(std::make_pair(name, entry));
}

// recursively fetch an entry's val
void SymbolTable::get_entry_val(std::string name, int &val) {
    for (auto it_b = block_stack.rbegin(); it_b!= block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) {
            val = it_entry->second.val;
            return;
        }
    }
    assert(false);
}

void SymbolTable::write_entry_val(std::string name, int val) {
    for (auto it_b = block_stack.rbegin(); it_b!= block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) {
            it_entry->second.val = val;
            return;
        }
    }
    assert(false);
}

void SymbolTable::push_block() {
    block_stack.push_back(symbol_table_block_t());
}

void SymbolTable::pop_block() { block_stack.pop_back(); }