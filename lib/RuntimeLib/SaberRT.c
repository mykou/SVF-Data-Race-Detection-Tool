/*
 * SaberRT.c
 *
 *  Created on: May 6, 2014
 *      Author: Yulei Sui
 */

#include "SaberRT.h"
#include <limits.h>
#include <assert.h>

#define DBG 1
#define DBOUT(x) if(DBG) x;

const char * CNRM = "\x1B[1;0m";
const char * CRED = "\x1B[1;31m";
const char * CGRN = "\x1B[1;32m";
const char * CYEL = "\x1B[1;33m";
const char * CBLU = "\x1B[1;34m";
const char * CPUR = "\x1B[1;35m";
const char * CCYA = "\x1B[1;36m";

// TODO : __attribute__ ((__packed__)) ?
typedef struct  {
	  void* addr;
	  size_t size;
}MDEntry;


/// Enlarge the memory if the arrays overflow
static const size_t  __MaxNumMDEntry=INT_MAX >> 6; // 33554432 (2^25 -1)
static size_t __NumAllocEntry = 0;
static size_t __NumFreedEntry = 0;

MDEntry MemAllocTable[__MaxNumMDEntry];
MDEntry MemFreeTable[__MaxNumMDEntry];

extern int pseudo_main(int argc, char **argv);

__WEAK_INLINE void __saber_free(void* addr, size_t size) {
	MDEntry* mdElem = &MemFreeTable[__NumFreedEntry++];
	mdElem->addr = addr;
	mdElem->size = size;
	printf("%s addr 0x%zx : saber reach free from source %ld %s \n", CGRN, (size_t)addr, size, CNRM);
}

__WEAK_INLINE void __saber_malloc(void* addr, size_t size) {
	MDEntry* mdElem = &MemAllocTable[__NumAllocEntry++];
	mdElem->addr = addr;
	mdElem->size = size;
	printf("%s addr 0x%zx : malloc at source %ld %s \n", CYEL, (size_t)addr, size, CNRM);
}

void statisticsPrint(){
	DBOUT(printf("total MD size = %zd \n", __NumAllocEntry))
}

void sizeofTypePrint(){
	DBOUT(printf("max MD size = %zd \n", __MaxNumMDEntry))
	DBOUT(printf("size_t = %ld,uint32_t = %ld, uint64_t = %ld, u long = %ld, void* = %ld\n", sizeof(size_t),sizeof(uint32_t),sizeof(uint64_t), sizeof(unsigned long), sizeof(void*)))
}

void MDInitialize(){

	sizeofTypePrint();
}

int main(int argc, char **argv){

	MDInitialize();

	int return_value = pseudo_main(argc, argv);

	statisticsPrint();

	return return_value;

}
