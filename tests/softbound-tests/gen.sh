for i in `ls *.c`
do
clang -c -emit-llvm $i -o $i.bc;
done
