all: test test-xmm test-ymm-sim

test: test.c
	clang -O3  -mavx test.c -o test.out

test-xmm: test-xmm.c
	clang -O3 -msse4 test-xmm.c -o test-xmm.out --static

test-ymm-sim: test-ymm-sim.c
	clang -O3 -mavx test-ymm-sim.c -o test-ymm-sim.out --static


clean:
	rm -rf test.out *~ *.out