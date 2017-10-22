

  clang -c -emit-llvm case3.c -o case3.bc


  ../../Debug+Asserts/bin/MemSafeC case3.bc

  llvm-ld -native -lm -lcrypt -lpthread case3.bc -o case3-memsafec.out -lm
  ./case3-memsafec.out

 llvm-dis case3.bc
 llvm-dis case3.bc.msc.bc



