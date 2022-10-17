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
            if (val->ty->tag != KOOPA_RTT_FUNCTION) continue;
            
            auto param_len = int(val->ty->data.function.params.len);
            length = std::max(4 * (param_len - 8), length);
            is_leaf_function = false;
        }
    }

    // second round: scan temporary variables
    for (size_t i = 0; i < bb_slice.len; i++) {
        auto bb = (koopa_raw_basic_block_t)bb_slice.buffer[i];
        auto val_slice = bb->insts;
        assert(val_slice.kind == KOOPA_RSIK_VALUE);

        for (size_t j = 0; j < val_slice.len; j++) {
            auto val = (koopa_raw_value_t)val_slice.buffer[j];
            if (val->ty->tag == KOOPA_RTT_UNIT) continue;

            StackInfo stack_info;
            stack_info.offset = length;
            length += 4;
            entries.insert(std::make_pair(val, stack_info));
        }
    }

    // save registers
    saved_registers.insert(std::make_pair("a0", StackInfo(length + 0)));
    saved_registers.insert(std::make_pair("sp", StackInfo(length + 4)));
    length += 8;

    // length align to 16
    length += saved_registers.size() * 4;
    length = ((length + 15) >> 4) << 4;
}

// Get offset from current sp
// (Support transforming negative offset)
int StackFrame::get_offset(koopa_raw_value_t val) {
    auto info = entries[val];
    auto offset = info.offset;
    if (offset < 0)
        offset = length + offset;
    return offset;
}