#include "ast.h"

bool ExpAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    // Sematically, you don't have to worry if exp is const.
    // An exp with var lval will pop false eventually.
    bool ret = dynamic_cast<CalcAST *>(binary_exp.get())
                   ->calc_val(irgen, result, calc_const);
    return ret;
}

bool BinaryExpAST::calc_val(IRGenerator &irgen, int &result,
                            bool calc_const) const {
    int lhs, rhs;
    bool ret = true;

    ret =
        ret &&
        dynamic_cast<CalcAST *>(l_exp.get())->calc_val(irgen, lhs, calc_const);
    ret =
        ret &&
        dynamic_cast<CalcAST *>(r_exp.get())->calc_val(irgen, rhs, calc_const);

    // calc_val doesn't dump inst, needless to short circuit
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
}

bool UnaryExpAST::calc_val(IRGenerator &irgen, int &result,
                           bool calc_const) const {
    bool ret;

    ret = dynamic_cast<CalcAST *>(unary_exp.get())
              ->calc_val(irgen, result, calc_const);
    if (op == "!") {
        result = !result;
    } else if (op == "-") {
        result = -result;
    } else if (op == "+") {
    } else {
        std::cerr << "Invalid op: " << op << std::endl;
        assert(false);  // Not Implemented
    }
    return ret;
}

bool PrimaryExpAST::calc_val(IRGenerator &irgen, int &result,
                             bool calc_const) const {
    if (type == PRIMARY_EXP_AST_TYPE_NUMBER) {
        result = number;
        return true;
    } else if (type == PRIMARY_EXP_AST_TYPE_LVAL) {
        return dynamic_cast<CalcAST *>(lval.get())
            ->calc_val(irgen, result, calc_const);
    } else {
        assert(false);
    }
}

bool LValAST::calc_val(IRGenerator &irgen, int &result, bool calc_const) const {
    // When using this lval, it should have already existed in symbol table,
    // No matter you're assigning a const or var lval.
    if (calc_const) {
        // if this is const calculation, you should only use const symbol,
        // and the symbol's value must have been initialized
        assert(irgen.symbol_table.is_const_var_entry(ident));
        result = irgen.symbol_table.get_const_var_val(ident);
        return true;
    } else {
        // otherwise, compiler checks constness and return.
        // For vardef, this provides optimization chance.
        result = irgen.symbol_table.get_const_var_val(ident);
        return irgen.symbol_table.is_const_var_entry(ident);
    }
}