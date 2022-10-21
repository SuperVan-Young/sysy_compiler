build/compiler -koopa debug/hello.c -o debug/hello.koopa
build/compiler -riscv debug/hello.c -o debug/hello.S

# link to riscv
clang debug/hello.S -c -o debug/hello.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld debug/hello.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o debug/hello
qemu-riscv32-static debug/hello