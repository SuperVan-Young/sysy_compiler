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

    std::fstream out;
    std::string koopa_file = "a.koopa";
    std::string assembly_file = "a.S";

    // ast -> IR
    out.open(koopa_file, ios::out);
    assert(out.is_open());
    IRGenerator irgen;
    ast->dump_koopa(irgen, out);
    out.close();

    // IR -> riscv assembly
    out.open(assembly_file, ios::out);
    assert(out.is_open());
    TargetCodeGenerator tcgen(koopa_file.c_str(), out);
    assert(!tcgen.dump_riscv());
    out.close();

    // copy to output
    std::fstream in;
    if (mode == "-koopa") {
        in.open(koopa_file, ios::in);
    } else if (mode == "-riscv" || mode == "-perf") {
        in.open(assembly_file, ios::in);
    } else {
        std::cerr << "Compiler: unrecognized mode " << mode << std::endl;
        return 1;
    }
    out.open(output, ios::out);
    assert(in);
    assert(out);
    out << in.rdbuf();
    in.close();
    out.close();

    std::cerr << "Compiler: Finished!" << std::endl;

    return 0;
}
