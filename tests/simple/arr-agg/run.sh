

  clang -c -emit-llvm case4.c -o case4.bc


  ../../Debug+Asserts/bin/MemSafeC case4.bc

  llvm-ld -native -lm -lcrypt -lpthread case4.bc -o case4-memsafec.out -lm
  ./case4-memsafec.out

 llvm-dis case4.bc
 llvm-dis case4.bc.msc.bc



