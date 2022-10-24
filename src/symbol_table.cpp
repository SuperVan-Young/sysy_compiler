#include "irgen.h"

// helper functions

// generate alias for a var/array
int SymbolTable::_get_alias(std::string name) {
    auto it_alias = alias_cnt.find(name);
    if (it_alias == alias_cnt.end()) {
        alias_cnt.insert(std::make_pair(name, 0));
        it_alias = alias_cnt.find(name);
    }
    return (it_alias->second)++;
}

// try to get local table and return true,
// otherwise return global table and false
bool SymbolTable::_get_local_table(symbol_table_block_t *&table) {
    auto it_b = block_stack.rbegin();
    if (it_b != block_stack.rend()) {
        table = &(*it_b);
        return true;
    } else {
        table = &global_table;
        return false;
    }
}

// return symbol entry if successful
bool SymbolTable::_get_entry(std::string name, SymbolTableEntry *&entry) {
    // recursive searching on local symbol table stack
    for (auto it_b = block_stack.rbegin(); it_b != block_stack.rend(); it_b++) {
        auto it_entry = it_b->find(name);
        if (it_entry != it_b->end()) {
            entry = &(it_entry->second);
            return true;
        }
    }
    // check global symbol table
    auto it_entry = global_table.find(name);
    if (it_entry != global_table.end()) {
        entry = &(it_entry->second);
        return true;
    }
    entry = nullptr;
    return false;
}

// insert new entry

void SymbolTable::insert_var_entry(std::string name) {
    symbol_table_block_t *table = nullptr;
    bool is_local = _get_local_table(table);
    assert(table->find(name) == table->end());  // no duplication

    SymbolTableEntry entry;
    entry.type = SYMBOL_TABLE_ENTRY_VAR;
    if (is_local) {
        entry.is_named = false;
        entry.alias = _get_alias(name);
    } else {
        entry.is_named = true;
        entry.alias = -1;
    }
    entry.is_const = false;

    table->insert(std::make_pair(name, entry));
}

void SymbolTable::insert_const_var_entry(std::string name, int val) {
    symbol_table_block_t *table = nullptr;
    bool is_local = _get_local_table(table);
    assert(table->find(name) == table->end());  // no duplication

    SymbolTableEntry entry;
    entry.type = SYMBOL_TABLE_ENTRY_VAR;
    if (is_local) {
        entry.is_named = false;
        entry.alias = _get_alias(name);
    } else {
        entry.is_named = true;
        entry.alias = -1;
    }
    entry.is_const = true;
    entry.val = val;

    table->insert(std::make_pair(name, entry));
}

void SymbolTable::insert_func_entry(std::string name, std::string func_type) {
    assert(global_table.find(name) == global_table.end());

    SymbolTableEntry entry;
    entry.type = SYMBOL_TABLE_ENTRY_FUNC;
    entry.func_type = func_type;
    assert(func_type == "int" || func_type == "void");

    global_table.insert(std::make_pair(name, entry));
}

void SymbolTable::insert_array_entry(std::string name,
                                     std::vector<int> array_size) {
    symbol_table_block_t *table = nullptr;
    bool is_local = _get_local_table(table);
    assert(table->find(name) == table->end());  // no duplication

    SymbolTableEntry entry;
    entry.type = SYMBOL_TABLE_ENTRY_ARRAY;
    if (is_local) {
        entry.is_named = false;
        entry.alias = _get_alias(name);
    } else {
        entry.is_named = true;
        entry.alias = -1;
    }
    entry.is_const = false;
    entry.array_size = array_size;

    table->insert(std::make_pair(name, entry));
}

// fetch entry info

symbol_table_entry_type_t SymbolTable::get_entry_type(std::string name) {
    SymbolTableEntry *entry = nullptr;
    assert(_get_entry(name, entry));
    return entry->type;
}

bool SymbolTable::is_global_symbol_table() {
    return (block_stack.size() == 0);
}

bool SymbolTable::is_const_var_entry(std::string name) {
    SymbolTableEntry *entry = nullptr;
    assert(_get_entry(name, entry));
    assert(entry->type == SYMBOL_TABLE_ENTRY_VAR);
    return entry->is_const;
}

int SymbolTable::get_const_var_val(std::string name) {
    SymbolTableEntry *entry = nullptr;
    assert(_get_entry(name, entry));
    assert(entry->type == SYMBOL_TABLE_ENTRY_VAR);
    assert(entry->is_const);
    return entry->val;
}

std::string SymbolTable::get_var_name(std::string name) {
    SymbolTableEntry *entry = nullptr;
    assert(_get_entry(name, entry));
    assert(entry->type == SYMBOL_TABLE_ENTRY_VAR);
    std::string ret = "";
    if (entry->is_named)
        ret += "@";
    else
        ret += "%";
    ret += name;
    if (entry->alias >= 0) ret += ("_" + std::to_string(entry->alias));
    return ret;
}

std::string SymbolTable::get_array_name(std::string name) {
    SymbolTableEntry *entry = nullptr;
    assert(_get_entry(name, entry));
    assert(entry->type == SYMBOL_TABLE_ENTRY_ARRAY);
    std::string ret = "";
    if (entry->is_named)
        ret += "@";
    else
        ret += "%";
    ret += name;
    if (entry->alias >= 0) ret += ("_" + std::to_string(entry->alias));
    return ret;
}

std::string SymbolTable::get_func_entry_type(std::string name) {
    auto it_entry = global_table.find(name);
    assert(it_entry != global_table.end());
    assert(it_entry->second.type == SYMBOL_TABLE_ENTRY_FUNC);
    return it_entry->second.func_type;
}

// basic block stacking

void SymbolTable::push_block() {
    block_stack.push_back(symbol_table_block_t());
}

void SymbolTable::pop_block() { block_stack.pop_back(); }
