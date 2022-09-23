#pragma once

#include <cstring>
#include <stack>

/**
 * @brief
 * Save information when generating koopa IR
 */
class IRGenerator {
   public:
    int cnt_val;
    std::stack<std::string> stack_val;
    int cnt_block;

    IRGenerator() {
        cnt_val = 0;
        cnt_block = 0;
    }

    std::string new_val() {
        auto val = cnt_val++;
        return "%" + std::to_string(val);
    }
};