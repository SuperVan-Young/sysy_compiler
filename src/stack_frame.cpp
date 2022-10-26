#include "tcgen.h"

StackFrame::StackFrame(koopa_raw_function_t func) {
    auto bb_slice = func->bbs;
    assert(bb_slice.kind == KOOPA_RSIK_BASIC_BLOCK);

    bool is_leaf_function = true;

    // first round: calculate length A
    for (size_t i = 0; i < bb_slice.len; i++) {
        auto bb = (koopa_raw_basic_block_t)bb_slice.buffer[i];
        auto val_slice = bb->insts;
        assert(val_slice.kind == KOOPA_RSIK_VALUE);

        for (size_t j = 0; j < val_slice.len; j++) {
            auto val = (koopa_raw_value_t)val_slice.buffer[j];
            if (val->kind.tag != KOOPA_RVT_CALL) continue;

            auto param_len = int(val->kind.data.call.args.len);
            length = std::max(4 * (param_len - 8), length);
            is_leaf_function = false;
        }
    }

    // second round: scan temporary variables & alloc memory
    for (size_t i = 0; i < bb_slice.len; i++) {
        auto bb = (koopa_raw_basic_block_t)bb_slice.buffer[i];
        auto val_slice = bb->insts;
        assert(val_slice.kind == KOOPA_RSIK_VALUE);

        for (size_t j = 0; j < val_slice.len; j++) {
            auto val = (koopa_raw_value_t)val_slice.buffer[j];

            StackInfo val_info;
            val_info.size = get_koopa_raw_value_size(val->ty);
            _insert_koopa_value(val, val_info);

            if (val->kind.tag == KOOPA_RVT_ALLOC) {
                std::cerr << val->name << std::endl;
                assert(val->ty->tag == KOOPA_RTT_POINTER);
                StackInfo alloc_info;
                alloc_info.size = get_koopa_raw_value_size(val->ty->data.pointer.base);
                _insert_alloc_memory(val->name, alloc_info);

            }
        }
    }

    // save registers
    _insert_saved_registers("ra", StackInfo());
    _insert_saved_registers("s0", StackInfo());

    // length align to 16
    length = ((length + 15) >> 4) << 4;
}

// insert info entry (add type and offset, handle length automatically)

void StackFrame::_insert_saved_registers(std::string name, StackInfo info) {
    info.type = STACK_INFO_SAVED_REGISTER;
    assert(saved_registers.find(name) == saved_registers.end());
    info.offset = length;
    length += info.size;
    saved_registers.insert(std::make_pair(name, info));
}

void StackFrame::_insert_koopa_value(koopa_raw_value_t val, StackInfo info) {
    info.type = STACK_INFO_KOOPA_VALUE;
    assert(koopa_values.find(val) == koopa_values.end());
    info.offset = length;
    length += info.size;
    koopa_values.insert(std::make_pair(val, info));
}

void StackFrame::_insert_alloc_memory(std::string name, StackInfo info) {
    info.type = STACK_INFO_ALLOC_MEMORY;
    assert(alloc_memory.find(name) == alloc_memory.end());
    info.offset = length;
    length += info.size;
    alloc_memory.insert(std::make_pair(name, info));
}

// Get offset from current sp
// (Support transforming negative offset)
StackInfo StackFrame::get_saved_register(std::string name) {
    assert(saved_registers.find(name) != saved_registers.end());
    auto info = saved_registers[name];
    if (info.offset < 0) info.offset += length;
    return info;
}

StackInfo StackFrame::get_koopa_value(koopa_raw_value_t val) {
    assert(koopa_values.find(val) != koopa_values.end());
    auto info = koopa_values[val];
    if (info.offset < 0) info.offset += length;
    return info;
}

StackInfo StackFrame::get_alloc_memory(std::string name) {
    assert(alloc_memory.find(name) != alloc_memory.end());
    auto info = alloc_memory[name];
    if (info.offset < 0) info.offset += length;
    return info;
}