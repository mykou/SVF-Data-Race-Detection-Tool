/*
 * Simple race check
 * Author: dye
 * Date: 28/04/2017
 */
#ifndef SRE_H
#define SRE_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#define VOID void
#ifndef NULL
#define NULL (0)
#endif
#define VALID (1)

typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef char CHAR;

/*
 * SRE lock
 */
typedef VOID* SRE_LOCK_ID;
typedef SRE_LOCK_ID SRE_SPECLOCKID_T;

EXTERNC VOID SRE_SplSpecLockEx(SRE_SPECLOCKID_T LockId, char *file, UINT32 line);
#define SRE_SplSpecLock(LockId) SRE_SplSpecLockEx(LockId, __FILE__, __LINE__)
EXTERNC VOID SRE_SplSpecUnlockEx(SRE_SPECLOCKID_T LockId, char *file, UINT32 line);
#define SRE_SplSpecUnlock(LockId) SRE_SplSpecUnlockEx(LockId, __FILE__, __LINE__)

EXTERNC UINT32 SRE_SplSpecCreate(char *name, UINT32 type, SRE_LOCK_ID *lockId);

#define OS_SEC_ALW_INLINE inline
#define INLINE inline
#define SRE_SPINLOCK_TYPE 1
#define SRE_RWLOCK_TYPE 2
OS_SEC_ALW_INLINE INLINE SRE_LOCK_ID SRE_LockCreate(UINT32 type) {
    SRE_LOCK_ID lockId = NULL;
    UINT32 ret;
    ret = SRE_SplSpecCreate(NULL, type, &lockId);
    if (0 != ret) {
        return NULL;
    } else {
        return lockId;
    }
}

#undef EXTERNC
#endif
