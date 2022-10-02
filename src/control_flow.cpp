#include "irgen.h"

void ControlFlow::insert_info(std::string name, BasicBlockInfo info) {
    cfg.insert(std::make_pair(name, info));
}

void ControlFlow::switch_control() {
    assert(to_next_block);
    BasicBlockInfo &info = cfg[cur_block];
    assert(info.ending != BASIC_BLOCK_ENDING_STATUS_NULL);
    cur_block = info.next;
    to_next_block = false;
}

basic_block_ending_status_t ControlFlow::check_ending_status() {
    auto &info = cfg[cur_block];
    return info.ending;
}

void ControlFlow::modify_ending_status(basic_block_ending_status_t status) {
    auto &info = cfg[cur_block];
    assert(info.ending == BASIC_BLOCK_ENDING_STATUS_NULL);
    assert(status != BASIC_BLOCK_ENDING_STATUS_NULL);
    info.ending = status;
}