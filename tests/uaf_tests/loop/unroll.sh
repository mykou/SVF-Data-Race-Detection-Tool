#!/bin/bash
FILE=$(basename $1 .c)
BC=$FILE".bc"
OPT=$FILE".opt"
UNROLL=$FILE".unroll"
clang -S -g -emit-llvm $1 -o $BC
opt -mem2reg $BC -o $OPT
opt -mem2reg -simplifycfg -loops -lcssa -loop-simplify -loop-rotate -loop-unroll -unroll-count=2 -unroll-allow-partial $BC -o $UNROLL
rm $BC
llvm-dis $OPT
llvm-dis $UNROLL
stc -uaf -mb $UNROLL
