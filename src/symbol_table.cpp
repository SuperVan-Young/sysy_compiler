#include "irgen.h"

// recursively check if a symbol exists
bool SymbolTable::exist_entry(std::string name) {
    // recursive searching
    for (auto it_b = block_stack.rbegin(); it_b != block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) return true;
    }
    return false;
}

// recursively check if a symbol is const
bool SymbolTable::is_const_entry(std::string name) {
    // recursive searching
    for (auto it_b = block_stack.rbegin(); it_b != block_stack.rend(); it_b++) {
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

    // add alias
    auto it_alias = alias_cnt.find(name);
    if (it_alias == alias_cnt.end()) {
        alias_cnt[name] = 1;
        entry.alias = 0;
    } else {
        entry.alias = alias_cnt[name]++;
    }

    it_b->insert(std::make_pair(name, entry));
}

// recursively fetch an entry's val
int SymbolTable::get_entry_val(std::string name) {
    for (auto it_b = block_stack.rbegin(); it_b != block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) {
            return it_entry->second.val;
        }
    }
    assert(false);
}

void SymbolTable::write_entry_val(std::string name, int val) {
    for (auto it_b = block_stack.rbegin(); it_b != block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) {
            it_entry->second.val = val;
            return;
        }
    }
    assert(false);
}

void SymbolTable::push_block(bool by_func_def) {
    if (by_func_def) {
        assert(!pending_push);
        pending_push = true;
        block_stack.push_back(symbol_table_block_t());
    } else {
        if (pending_push) {
            pending_push = false;
        } else {
            block_stack.push_back(symbol_table_block_t());
        }
    }
}

void SymbolTable::pop_block() { block_stack.pop_back(); }

std::string SymbolTable::get_aliased_name(std::string name, bool with_prefix) {
    for (auto it_b = block_stack.rbegin(); it_b != block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) {
            std::string prefix;
            if (it_entry->second.is_func_param)
                prefix = "%";
            else
                prefix = "@";
            auto suffix = std::to_string(it_entry->second.alias);
            if (with_prefix)
                return prefix + name + "_" + suffix;
            else
                return name + "_" + suffix;
        }
    }
    assert(false);
}

void SymbolTable::insert_func_entry(std::string name,
                                    FuncSymbolTableEntry entry) {
    auto it = funcs.find(name);
    assert(it == funcs.end());
    funcs.insert(std::make_pair(name, entry));
}

std::string SymbolTable::get_func_entry_type(std::string name) {
    auto it = funcs.find(name);
    assert(it != funcs.end());
    return it->second.func_type;
}