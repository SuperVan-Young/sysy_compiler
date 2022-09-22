#include <ast.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    assert(argc == 5);
    auto mode = std::string(argv[1]);
    auto input = std::string(argv[2]);
    auto output = std::string(argv[4]);

    yyin = fopen(input.c_str(), "r");
    assert(yyin);

    // lex & parse
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    // dumping koopa IR
    std::fstream out;
    const std::string koopa_log = "koopa.txt";
    if (mode == "-koopa")
        out.open(output, ios::out);
    else
        out.open(koopa_log, ios::out);
    assert(out.is_open());
    ast->dump_koopa(out);
    out.close();

    // dumping riscv
    if (mode == "-riscv") {
        // TODO: add compiler backend
    }

    return 0;
}
