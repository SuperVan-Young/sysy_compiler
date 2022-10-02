#include "irgen.h"

void ControlFlow::get_info(std::string name, BasicBlockInfo &info) {
    info = cfg[name];
}

void ControlFlow::insert_info(std::string name, BasicBlockInfo info) {
    cfg.insert(std::make_pair(name, info));
}