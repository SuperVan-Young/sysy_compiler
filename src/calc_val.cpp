#include "ast.h"

bool ExpAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    // Sematically, you don't have to worry if exp is const.
    // An exp with var lval will pop false eventually.
    bool ret =
        dynamic_cast<CalcAST *>(binary_exp.get())->calc_val(irgen, result, calc_const);
    return ret;
}

bool BinaryExpAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    int lhs, rhs;
    bool ret = true;

    if (type == BINARY_EXP_AST_TYPE_0) {
        ret = dynamic_cast<CalcAST *>(r_exp.get())->calc_val(irgen, result, calc_const);
        return ret;
    } else if (type == BINARY_EXP_AST_TYPE_1) {
        ret = ret && dynamic_cast<CalcAST *>(l_exp.get())->calc_val(irgen, lhs, calc_const);
        ret = ret && dynamic_cast<CalcAST *>(r_exp.get())->calc_val(irgen, rhs, calc_const);

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

bool UnaryExpAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    bool ret;

    if (type == UNARY_EXP_AST_TYPE_0) {
        ret =
            dynamic_cast<CalcAST *>(primary_exp.get())->calc_val(irgen, result, calc_const);
    } else if (type == UNARY_EXP_AST_TYPE_1) {
        ret = dynamic_cast<CalcAST *>(unary_exp.get())->calc_val(irgen, result, calc_const);
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
    return ret;
}

bool PrimaryExpAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    if (type == PRIMARY_EXP_AST_TYPE_0) {
        return dynamic_cast<CalcAST *>(exp.get())->calc_val(irgen, result, calc_const);
    } else if (type == PRIMARY_EXP_AST_TYPE_1) {
        result = number;
        return true;
    } else if (type == PRIMARY_EXP_AST_TYPE_2) {
        return dynamic_cast<CalcAST *>(lval.get())->calc_val(irgen, result, calc_const);
    } else {
        assert(false);
    }
}

bool LValAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    // When using this lval, it should have already existed in symbol table,
    // No matter you're assigning a const or var lval.
    if (!irgen.symbol_table.exist_entry(ident))
        assert(false);

    if (calc_const) {
        // if this is const calculation, you should only use const symbol,
        // and the symbol's value must have been initialized
        assert(irgen.symbol_table.is_const_entry(ident));
        assert(irgen.symbol_table.get_entry_val(ident, result));
        return true;
    } else {
        // otherwise, compiler checks constness and return.
        // For vardef, this provides optimization chance.
        irgen.symbol_table.get_entry_val(ident, result);
        return irgen.symbol_table.is_const_entry(ident);
    }
}