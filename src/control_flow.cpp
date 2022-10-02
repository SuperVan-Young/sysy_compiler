#include "irgen.h"

void ControlFlow::insert_info(std::string name, BasicBlockInfo info) {
    cfg.insert(std::make_pair(name, info));
}

basic_block_ending_status_t ControlFlow::check_ending_status(std::string name) {
    if (name == "")
        name = cur_block;
    auto &info = cfg[name];
    return info.ending;
}

void ControlFlow::modify_ending_status(basic_block_ending_status_t status) {
    auto &info = cfg[cur_block];
    assert(info.ending == BASIC_BLOCK_ENDING_STATUS_NULL);
    assert(status != BASIC_BLOCK_ENDING_STATUS_NULL);
    info.ending = status;
}

std::string ControlFlow::get_dst_break(std::string name) {
    if (name == "")
        name = cur_block;
    auto &info = cfg[name];
    return info.dst_break;
}

std::string ControlFlow::get_dst_continue(std::string name) {
    if (name == "")
        name = cur_block;
    auto &info = cfg[name];
    return info.dst_continue;
}