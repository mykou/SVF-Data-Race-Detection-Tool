clang -c -emit-llvm case9.c -o case9.bc
#dvf -debug-only=regpt case9.bc
dvf case9.bc
#msc case9.bc
llvm-dis case9.bc
