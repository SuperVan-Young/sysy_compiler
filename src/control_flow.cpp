#include "irgen.h"

void ControlFlow::insert_if(std::string name_then, std::string name_end) {
    auto dst_break = cfg[cur_block].dst_break;
    auto dst_continue = cfg[cur_block].dst_continue;

    // inherit break/continue dsts
    auto then_block_info = BasicBlockInfo(name_end, dst_break, dst_continue);
    auto end_block_info = BasicBlockInfo("", dst_break, dst_continue);

    cfg.insert(make_pair(name_then, then_block_info));
    cfg.insert(make_pair(name_end, end_block_info));

    // add control edge, making sure then/end are all dumped
    add_control_edge(name_then, cur_block);
    add_control_edge(name_end, cur_block);
}

void ControlFlow::insert_if_else(std::string name_then, std::string name_else,
                                 std::string name_end) {
    auto dst_break = cfg[cur_block].dst_break;
    auto dst_continue = cfg[cur_block].dst_continue;

    auto then_block_info = BasicBlockInfo(name_end, dst_break, dst_continue);
    auto else_block_info = BasicBlockInfo(name_end, dst_break, dst_continue);
    auto end_block_info = BasicBlockInfo("", dst_break, dst_continue);

    cfg.insert(make_pair(name_then, then_block_info));
    cfg.insert(make_pair(name_else, else_block_info));
    cfg.insert(make_pair(name_end, end_block_info));

    // add control edge, making sure then/else are all dumped
    add_control_edge(name_then, cur_block);
    add_control_edge(name_else, cur_block);
}

void ControlFlow::insert_while(std::string name_entry, std::string name_body,
                               std::string name_end) {
    auto entry_block_info = BasicBlockInfo();
    // new dsts
    auto body_block_info = BasicBlockInfo(name_entry, name_end, name_entry);
    // old dsts
    auto end_block_info = BasicBlockInfo("", cfg[cur_block].dst_break,
                                         cfg[cur_block].dst_continue);

    cfg.insert(make_pair(name_entry, entry_block_info));
    cfg.insert(make_pair(name_body, body_block_info));
    cfg.insert(make_pair(name_end, end_block_info));

    add_control_edge(name_entry, cur_block);  // make sure we dump name_entry
    add_control_edge(name_body, name_entry);  // similar to if-else
    add_control_edge(name_end, name_entry);
}

void ControlFlow::init_entry_block(std::string name, std::ostream &out) {
    auto block_info = BasicBlockInfo();
    cfg[name] = block_info;
    cur_block = name;
    out << name << ":" << std::endl;
}

// switch to target control flow.
// It doesn't automatically wrap up current control block,
// but will check if it has ended.
// It also checks if current block is reached by any other block.
// If not, nothing will be printed and will return false.
// Return if switch completes successfully.
bool ControlFlow::switch_control_flow(std::string name, std::ostream &out) {
    assert(cfg[cur_block].ending != BASIC_BLOCK_ENDING_STATUS_NULL);
    if (cfg[name].edge_in.size() == 0) {
        cfg[name].ending = BASIC_BLOCK_ENDING_STATUS_UNREACHABLE;
        return false;
    }
    cur_block = name;
    out << name << ":" << std::endl;
    return true;
}

// Check a block's ending status
// Used to prune unreachable insts and add default return
basic_block_ending_status_t ControlFlow::check_ending_status() {
    auto &info = cfg[cur_block];
    return info.ending;
}

// modify current block's status
void ControlFlow::modify_ending_status(basic_block_ending_status_t status) {
    auto &info = cfg[cur_block];
    assert(info.ending == BASIC_BLOCK_ENDING_STATUS_NULL);
    assert(status != BASIC_BLOCK_ENDING_STATUS_NULL);
    info.ending = status;
}

void ControlFlow::add_control_edge(std::string dst, std::string src) {
    if (src == "") src = cur_block;
    auto &src_info = cfg[src];
    auto &dst_info = cfg[dst];
    src_info.edge_out.push_back(dst);
    dst_info.edge_in.push_back(src);
}

void ControlFlow::_break(std::ostream &out) {
    auto dst_break = cfg[cur_block].dst_break;
    assert(dst_break != "");
    out << "  jump " << dst_break << std::endl;
    modify_ending_status(BASIC_BLOCK_ENDING_STATUS_BREAK);
    add_control_edge(dst_break);
}

void ControlFlow::_continue(std::ostream &out) {
    auto dst_continue = cfg[cur_block].dst_continue;
    assert(dst_continue != "");
    out << "  jump " << dst_continue << std::endl;
    modify_ending_status(BASIC_BLOCK_ENDING_STATUS_CONTINUE);
    add_control_edge(dst_continue);
}