#pragma once
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "irgen.h"

// Base class of AST
class BaseAST {
   public:
    virtual ~BaseAST() = default;

    virtual void dump_koopa(IRGenerator &irgen, std::ostream &out) const = 0;
};

// CompUnit       ::= FuncDef
class CompUnitAST : public BaseAST {
   public:
    std::unique_ptr<BaseAST> func_def;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// Decl          ::= ConstDecl | VarDecl
// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";"
// VarDecl       ::= BType VarDef {"," VarDef} ";"
class DeclAST : public BaseAST {
   public:
    bool is_const;
    std::string btype;  // only int
    std::vector<std::unique_ptr<BaseAST>> defs;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// ConstDef      ::= IDENT "=" ConstInitVal
// VarDef        ::= IDENT | IDENT "=" InitVal
class DeclDefAST : public BaseAST {
   public:
    bool is_const;
    std::string ident;
    std::unique_ptr<BaseAST> init_val;  // could be null for var
    DeclDefAST *next;                   // for optional defs

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// ConstInitVal  ::= ConstExp
// InitVal       ::= Exp
// TODO: This will be expanded for array
class InitValAST : public BaseAST {
   public:
    bool is_const;
    std::unique_ptr<BaseAST> exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override {
        assert(false);  // this function shouldn't be called
    }
};

// FuncDef       ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST {
   public:
    std::string func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

// Block         ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {
   public:
    std::vector<std::unique_ptr<BaseAST>> items;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    BLOCK_ITEM_AST_TYPE_0 = 0,  // decl
    BLOCK_ITEM_AST_TYPE_1,      // stmt
} block_item_ast_type;

// BlockItem     ::= Decl | Stmt;
class BlockItemAST : public BaseAST {
   public:
    block_item_ast_type type;
    std::unique_ptr<BaseAST> item;
    BlockItemAST *next;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

typedef enum {
    STMT_AST_TYPE_ASSIGN,    // assign
    STMT_AST_TYPE_RETURN,    // return
    STMT_AST_TYPE_EXP,       // exp
    STMT_AST_TYPE_BLOCK,     // block
    STMT_AST_TYPE_IF,        // if
    STMT_AST_TYPE_WHILE,     // while
    STMT_AST_TYPE_BREAK,     // break
    STMT_AST_TYPE_CONTINUE,  // continue
} stmt_ast_type;

// Stmt          ::= LVal "=" Exp ";"
//                 | [Exp] ";"
//                 | Block
//                 | "if" "(" Exp ")" Stmt ["else" Stmt]
//                 | "return" [Exp] ";"
class StmtAST : public BaseAST {
   public:
    stmt_ast_type type;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> then_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> do_stmt;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
};

class CalcAST : public BaseAST {
   public:
    // Calculate AST's value, and store the result in the given reference.
    // calc_const forces to use const value, if not, raises errors.
    // Return true if we can determine that the calculated value is const
    virtual bool calc_val(IRGenerator &irgen, int &result,
                          bool calc_const) const = 0;
};

// ConstExp      ::= Exp
// Exp           ::= LOrExp
class ExpAST : public CalcAST {
   public:
    bool is_const;
    std::unique_ptr<BaseAST> binary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result,
                  bool calc_const) const override;
};

// MulExp        ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
// AddExp        ::= MulExp | AddExp ("+" | "-") MulExp;
// RelExp        ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
// EqExp         ::= RelExp | EqExp ("==" | "!=") RelExp;
// LAndExp       ::= EqExp | LAndExp "&&" EqExp;
// LOrExp        ::= LAndExp | LOrExp "||" LAndExp;
class BinaryExpAST : public CalcAST {
   public:
    std::string op;
    std::unique_ptr<BaseAST> l_exp;
    std::unique_ptr<BaseAST> r_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result,
                  bool calc_const) const override;
};

// UnaryExp      ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public CalcAST {
   public:
    std::string op;
    std::unique_ptr<BaseAST> unary_exp;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result,
                  bool calc_const) const override;
};

typedef enum {
    PRIMARY_EXP_AST_TYPE_NUMBER,  // number
    PRIMARY_EXP_AST_TYPE_LVAL,    // lval
} primary_exp_ast_type;

class PrimaryExpAST : public CalcAST {
   public:
    primary_exp_ast_type type;
    int number;
    std::unique_ptr<BaseAST> lval;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result,
                  bool calc_const) const override;
};

class LValAST : public CalcAST {
   public:
    std::string ident;

    void dump_koopa(IRGenerator &irgen, std::ostream &out) const override;
    bool calc_val(IRGenerator &irgen, int &result,
                  bool calc_const) const override;
};