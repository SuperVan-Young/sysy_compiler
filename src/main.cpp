
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

#include "ast.h"
#include "tcgen.h"

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
    const std::string koopa_log = ".koopa.txt";
    if (mode == "-koopa")
        out.open(output, ios::out);
    else
        out.open(koopa_log, ios::out);
    assert(out.is_open());
    IRGenerator irgen;
    ast->dump_koopa(irgen, out);
    out.close();

    // dumping riscv
    if (mode == "-riscv") {
        out.open(output, std::ios::out);
        assert(out.is_open());
        TargetCodeGenerator backend(koopa_log.c_str(), out);
        int ret = backend.dump_riscv();
        assert(!ret);
    }

    return 0;
}
