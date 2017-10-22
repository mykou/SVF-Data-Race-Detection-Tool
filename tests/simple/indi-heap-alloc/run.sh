clang -c -emit-llvm case8.c -o case8.bc
dvf -debug-only=regpt case8.bc
dvf -print-pts -dump-pag-name case8.bc
llvm-dis case8.bc
