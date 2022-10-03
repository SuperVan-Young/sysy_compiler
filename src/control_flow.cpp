#include "irgen.h"

void ControlFlow::insert_info(std::string name, BasicBlockInfo info) {
    cfg.insert(std::make_pair(name, info));
}

basic_block_ending_status_t ControlFlow::check_ending_status(std::string name) {
    if (name == "") name = cur_block;
    auto &info = cfg[name];
    return info.ending;
}

void ControlFlow::modify_ending_status(basic_block_ending_status_t status,
                                       std::string name) {
    if (name == "")
        name = cur_block;
    auto &info = cfg[name];
    assert(info.ending == BASIC_BLOCK_ENDING_STATUS_NULL);
    assert(status != BASIC_BLOCK_ENDING_STATUS_NULL);
    info.ending = status;
}

std::string ControlFlow::get_dst_break(std::string name) {
    if (name == "") name = cur_block;
    auto &info = cfg[name];
    return info.dst_break;
}

std::string ControlFlow::get_dst_continue(std::string name) {
    if (name == "") name = cur_block;
    auto &info = cfg[name];
    return info.dst_continue;
}

void ControlFlow::add_control_edge(std::string dst, std::string src) {
    if (src == "") src = cur_block;
    auto &src_info = cfg[src];
    auto &dst_info = cfg[dst];
    src_info.edge_out.push_back(dst);
    dst_info.edge_in.push_back(src);
}

bool ControlFlow::is_reachable(std::string name) {
    if (name == "") name = cur_block;
    auto &info = cfg[name];
    auto len = info.edge_in.size();
    return len != 0;
}