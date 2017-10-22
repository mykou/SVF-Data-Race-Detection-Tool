/*
 * InstrumentLib.c
 *
 *  Created on: Jun 6, 2013
 *      Author: Yulei Sui
 */

#include "BoundsRT.h"
#include <limits.h>
#include <assert.h>
#include <string.h>

#define DBG 1
#define DBOUT(x) if(DBG) x;

//typedef enum { false, true } bool;

/*
 *   kernel space:          FFFF,8000,0000,0000--- FFFF,FFFF,FFFF,FFFF
 *   unexplored space:  0000,7FFF,FFFF,FFFF --- FFFF,8000,0000,0000
 *   user space:            0000,0000,0000,0000 --- 0000,7FFF,FFFF,FFFF
 */
// size_t in 64bit machine is 8 bytes
static size_t __storeCounter = 0;
static size_t __loadCounter = 0;
static size_t __totalNumMDEntry = 0;

/// Enlarge the memory if the arrays overflow
static const size_t  __MaxNumMDEntry=INT_MAX >> 6; // 33554432 (2^25 -1)
static const size_t  __MaxNumPTRToMDEntry= INT_MAX >> 5; // 67108863 (2^26 - 1)
static const size_t  __MaxNumPTRToPropInfoEntry= INT_MAX >> 5; // 67108863 (2^26 - 1)

// TODO : __attribute__ ((__packed__)) ?
typedef struct  {
	  void* base;
	  void* bound;
	  size_t key;
	  void* lock;
}MDEntry;

typedef struct  {
	  void* base;
	  void* bound;
}PropInfo;

/// MDTable contains all Metadata during runtime
MDEntry* MDTable = NULL;

/// PTRToPropInfoTable associate ptr with metadata (MDEntry*) information during propagation
PropInfo* PTRToPropInfoTable  = NULL;

/// PTRToMDTable associate ptr with metadata (MDEntry*) information during propagation
MDEntry** PTRToMDTable  = NULL;

extern int pseudo_main(int argc, char **argv);

__WEAK_INLINE void
__metadataStoreCounter(){
	__storeCounter++;
}

__WEAK_INLINE void
__metadataLoadCounter(){
	__loadCounter++;
}

__WEAK_INLINE MDEntry*
__findMetaData(void* ptr){
	for (size_t id = 0; id < __totalNumMDEntry; ++id) {
    	MDEntry* mdElem = &MDTable[id];
        if (ptr >= mdElem->base && ptr < mdElem->bound) {
            return mdElem;
        }
    }
    return NULL;
}

__WEAK_INLINE MDEntry*
__getMDEntryFromCPtr(void* ptr){

	size_t ptrIdx = (size_t)ptr;
	size_t idx = ((ptrIdx >> 3) & 0x3fffff);

	assert(idx < __MaxNumPTRToMDEntry && "out of max number of PTRToMDEntry table limit");

	return PTRToMDTable[idx];
}

__WEAK_INLINE void
__setMDEntryFromCPtr(void* ptr, MDEntry* entry){

	size_t ptrIdx = (size_t)ptr;
	size_t idx = ((ptrIdx >> 3) & 0x3fffff);

	assert(idx < __MaxNumPTRToMDEntry && "out of max number of PTRToMDEntry table limit");

	PTRToMDTable[idx] = entry;
}

__WEAK_INLINE PropInfo*
__getPropInfoFromCPtr(void* ptr){

	size_t ptrIdx = (size_t)ptr;
	size_t idx = ((ptrIdx >> 3) & 0x3fffff);

	assert(idx < __MaxNumPTRToPropInfoEntry && "out of max number of PTRToPropInfoEntry table limit");

	return &PTRToPropInfoTable[idx];
}

__WEAK_INLINE void
__setPropInfoFromCPtr(void* ptr, PropInfo* propInfoSrc ){

	size_t ptrIdx = (size_t)ptr;
	size_t idx = ((ptrIdx >> 3) & 0x3fffff);

	assert(idx < __MaxNumPTRToPropInfoEntry && "out of max number of PTRToPropInfoEntry table limit");

	PTRToPropInfoTable[idx].base = propInfoSrc->base;
	PTRToPropInfoTable[idx].bound = propInfoSrc->bound;
}

__WEAK_INLINE void
__metadataCheckInst(void* ptr, size_t accessSize){

	PropInfo* propInfo = __getPropInfoFromCPtr(ptr);
	assert(propInfo && "no metadata associated with this ptr!!");

	DBOUT(printf("ptr 0x%zx, access size %ld, [base 0x%zx , bound 0x%zx] \n", (size_t)ptr, accessSize,
			(size_t)propInfo->base, (size_t)propInfo->bound))

    if ((ptr < propInfo->base) || ((void*)((char*)ptr + accessSize) > propInfo->bound)) {
    	assert(0 && "bound overflow errors !!");
	}
}


__WEAK_INLINE void
__metadataLoadInst(void* ptrSrc,void* ptrDst){

	DBOUT(printf("load ptrSrc 0x%zx to ptrDst 0x%zx\n", (size_t)ptrSrc, (size_t)ptrDst))

	MDEntry* mdElem = __getMDEntryFromCPtr(ptrSrc);
	/// source pointer should always points-to a valid metadata
	assert(mdElem && "no metadata found!!");

	PropInfo* propInfo = __getPropInfoFromCPtr(ptrDst);

	propInfo->base = mdElem->base;
	propInfo->bound = mdElem->bound;

}

__WEAK_INLINE void
__metadataStoreInst(void* ptrSrc,void* ptrDst){

	DBOUT(printf("store ptrSrc 0x%zx to ptrDst 0x%zx\n", (size_t)ptrSrc, (size_t)ptrDst))

	MDEntry* mdElem = __getMDEntryFromCPtr(ptrDst);
	/// dst pointer should points-to a valid metadata
	assert(mdElem && "no metadata found!!");

	PropInfo* propInfo = __getPropInfoFromCPtr(ptrSrc);
	/// source pointer should always associate with metadata
	assert(propInfo && "no propInfo found, pointer not associated with metadata!!");

	mdElem->base = propInfo->base;
	mdElem->bound =  propInfo->bound;

}

__WEAK_INLINE void
__metadataPropagateInst(void* ptrSrc,void* ptrDst){

	PropInfo* propInfoSrc = __getPropInfoFromCPtr(ptrSrc);

	/// source pointer should always associate with metadata
	assert(propInfoSrc && "pointer not associated with metadata");

	__setPropInfoFromCPtr(ptrDst,propInfoSrc);
	DBOUT(printf("propagate md from ptrSrc 0x%zx to ptrDst 0x%zx with [ 0x%zx -- 0x%zx ]\n",
			(size_t)ptrSrc, (size_t)ptrDst, (size_t)propInfoSrc->base, (size_t)propInfoSrc->bound))

}

__WEAK_INLINE void
__metadataInitialInst(void* ptr, void* ptrBase, void* ptrBound){

	DBOUT(printf("ptr 0x%zx : base 0x%zx to bound 0x%zx\n", (size_t)ptr, (size_t)ptrBase, (size_t)ptrBound))

	assert(__totalNumMDEntry < __MaxNumMDEntry && "out of max number of Metadata table limit");

	MDEntry* mdElem = &MDTable[__totalNumMDEntry++];

	mdElem->base = ptrBase;
	mdElem->bound = ptrBound;

	PropInfo* propInfo = __getPropInfoFromCPtr(ptr);

	propInfo->base = ptrBase;
	propInfo->bound = ptrBound;

	__setMDEntryFromCPtr(ptr, mdElem);
}

void sizeofTypePrint(){
	DBOUT(printf("max PTR size = %zd \n", __MaxNumPTRToPropInfoEntry))
	DBOUT(printf("max MD size = %zd \n", __MaxNumMDEntry))
	DBOUT(printf("size_t = %ld,uint32_t = %ld, uint64_t = %ld, u long = %ld, void* = %ld\n", sizeof(size_t),sizeof(uint32_t),sizeof(uint64_t), sizeof(unsigned long), sizeof(void*)))
}

void statisticsPrint(){
	DBOUT(printf("total MD size = %zd \n", __totalNumMDEntry))
	DBOUT(printf("total load =%zd, total store=%zd\n",__loadCounter,__storeCounter))
}

void MDInitialize(){
	MDTable = malloc(sizeof(MDEntry) * __MaxNumMDEntry);
    memset(MDTable, 0, sizeof(MDEntry) * __MaxNumMDEntry);
    PTRToPropInfoTable = malloc(sizeof(PropInfo) * __MaxNumPTRToPropInfoEntry);
	memset(PTRToPropInfoTable ,0 , sizeof(PropInfo) * __MaxNumPTRToPropInfoEntry);
	PTRToMDTable = malloc(sizeof(MDEntry*) * __MaxNumPTRToMDEntry);
	memset(PTRToMDTable ,0 , sizeof(MDEntry*) * __MaxNumPTRToMDEntry);

	sizeofTypePrint();
}


__WEAK_INLINE void* __dvf_malloc(size_t size) {

  char* ptr = (char*)malloc(size);
  if(ptr == NULL){
    //TODO: do we need a assertion here?
	  assert(0 && "not enough memory to be allocated!!");
  }
  else{
    char* ptrBound = ptr + size;
    __metadataInitialInst(ptr,ptr,ptrBound);
  }
  return (void*)ptr;
}

int main(int argc, char **argv){

	MDInitialize();

	int return_value = pseudo_main(argc, argv);

	statisticsPrint();

	return return_value;

}
