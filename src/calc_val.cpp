#include "ast.h"

bool ExpAST::calc_val(IRGenerator &irgen, int &result) const {
    bool ret =
        dynamic_cast<CalcAST *>(binary_exp.get())->calc_val(irgen, result);
    return ret;
}

bool BinaryExpAST::calc_val(IRGenerator &irgen, int &result) const {
    int lhs, rhs;
    bool ret = true;

    if (type == BINARY_EXP_AST_TYPE_0) {
        ret = dynamic_cast<CalcAST *>(r_exp.get())->calc_val(irgen, result);
        return ret;
    } else if (type == BINARY_EXP_AST_TYPE_1) {
        ret = ret && dynamic_cast<CalcAST *>(l_exp.get())->calc_val(irgen, lhs);
        ret = ret && dynamic_cast<CalcAST *>(r_exp.get())->calc_val(irgen, rhs);

        if (op == "+") {
            result = lhs + rhs;
        } else if (op == "-") {
            result = lhs - rhs;
        } else if (op == "*") {
            result = lhs * rhs;
        } else if (op == "/") {
            result = lhs / rhs;
        } else if (op == "%") {
            result = lhs % rhs;
        } else if (op == "<") {
            result = lhs < rhs;
        } else if (op == ">") {
            result = lhs > rhs;
        } else if (op == "<=") {
            result = lhs <= rhs;
        } else if (op == ">=") {
            result = lhs >= rhs;
        } else if (op == "==") {
            result = lhs == rhs;
        } else if (op == "!=") {
            result = lhs != rhs;
        } else if (op == "&&") {
            result = lhs && rhs;
        } else if (op == "||") {
            result = lhs || rhs;
        } else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);
        }

        return ret;
    } else {
        assert(false);
    }
}

bool UnaryExpAST::calc_val(IRGenerator &irgen, int &result) const {
    bool ret;

    if (type == UNARY_EXP_AST_TYPE_0) {
        ret =
            dynamic_cast<CalcAST *>(primary_exp.get())->calc_val(irgen, result);
        return ret;
    } else if (type == UNARY_EXP_AST_TYPE_1) {
        ret = dynamic_cast<CalcAST *>(unary_exp.get())->calc_val(irgen, result);
        if (op == "!") {
            result = !result;
        } else if (op == "-") {
            result = -result;
        } else if (op == "+") {
        } else {
            std::cerr << "Invalid op: " << op << std::endl;
            assert(false);  // Not Implemented
        }
    } else {
        assert(false);
    }
}

bool PrimaryExpAST::calc_val(IRGenerator &irgen, int &result) const {
    if (type == PRIMARY_EXP_AST_TYPE_0) {
        return dynamic_cast<CalcAST *>(exp.get())->calc_val(irgen, result);
    } else if (type == PRIMARY_EXP_AST_TYPE_1) {
        result = number;
        return true;
    } else if (type == PRIMARY_EXP_AST_TYPE_2) {
        return dynamic_cast<CalcAST *>(lval.get())->calc_val(irgen, result);
    }
}

bool LValAST::calc_val(IRGenerator &irgen, int &result) const {
    // When using this lval, it should have already existed in symbol table
    // however, for future optimization, we let const exp handle exceptions
    if (!irgen.symbol_table.exist_entry(ident))
        return false;
    return irgen.symbol_table.get_entry_val(ident, result);
}