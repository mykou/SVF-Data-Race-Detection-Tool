/*
 * Wireless Scene Sample Code
 * Author: dye
 * Date: 21/04/2017
 */
#include "pthread.h"
#include "sre.h"


/*****************************
 *** Data types and macros ***
 *****************************/
#define __l2_shared__ __attribute__((section(".mc_shared")))
#define VOID void
#ifndef NULL
#define NULL (0)
#endif
#define VALID (1)

typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef char CHAR;

typedef struct {
    UINT32 uljobID;;
    void (*JSF_JOB_ENTRY_FUNC) (void *paraData);
} JSF_JOB_REG_INFO;

#define L2OS_MOD_MSG_HEADER \
UINT16 usReceiverPid;\
UINT16 usSenderPid;

/**
 * Lock ID
 */
typedef VOID* SRE_LOCK_ID;
typedef SRE_LOCK_ID SRE_SPECLOCKID_T;

extern VOID SRE_SplSpecLockEx(SRE_SPECLOCKID_T LockId, char *file, UINT32 line);
#define SRE_SplSpecLock(LockId) SRE_SplSpecLockEx(LockId, __FILE__, __LINE__)
extern VOID SRE_SplSpecUnlockEx(SRE_SPECLOCKID_T LockId, char *file, UINT32 line);
#define SRE_SplSpecUnlock(LockId) SRE_SplSpecUnlockEx(LockId, __FILE__, __LINE__)

#define L2OS_SpinLock(LOCK) SRE_SplSpecLock(LOCK)
#define L2OS_SpinUnLock(LOCK) SRE_SplSpecUnlock(LOCK)


typedef enum {
    USER_JOB_RELEASE1   = 0,
    USER_JOB_RELEASE2   = 1,
    USER_JOB_RELEASE3   = 2,
    USER_JOB_UPDATE_STATE   = 3,
} USER_JOB_ENUM;

VOID User_Release1(VOID *pMsg);
VOID User_Release2(VOID *pMsg);
VOID User_Release3(VOID *pMsg);
VOID User_UpdateState(VOID *pMsg);

JSF_JOB_REG_INFO __l2_shared__ g_stUserJobTable[] =
{
        {USER_JOB_RELEASE1, User_Release1},
        {USER_JOB_RELEASE2, User_Release2},
        {USER_JOB_RELEASE3, User_Release3},
        {USER_JOB_UPDATE_STATE, User_UpdateState},
};

typedef struct
{
    UINT32 InstanceId;
    UINT32 CellId;
    UINT32 LastSduCnt;
    UINT32 CheckCount;
    UINT32 State;
} MUM_ENTITY_STRU;
MUM_ENTITY_STRU __l2_shared__ *gx_pstUserEntityTable = NULL;

MUM_ENTITY_STRU* MUM_GetInstance(UINT32 instanceId)
{
    return &gx_pstUserEntityTable[instanceId];
}

volatile UINT32 g_ReleaseCounter;
volatile SRE_LOCK_ID gCounterLock;

/* Continue to send message */
typedef struct {
    L2OS_MOD_MSG_HEADER
    UINT32 ulInstanceId;
    UINT32 ulCellId;
} USER_JOB_STRU;

VOID UserMgmt_RemoveUser1(UINT32 cellId, UINT32 userId);
VOID UserMgmt_RemoveUser2(UINT32 cellId, UINT32 userId);
VOID UserMgmt_RemoveUser3(UINT32 cellId, UINT32 userId);
VOID SRE_AtomicAddU32(UINT32 *raw, INT32 incr);



VOID User_Release1(VOID *pMsg) {
    MUM_ENTITY_STRU* pstUser = MUM_GetInstance(((USER_JOB_STRU*)pMsg)->ulInstanceId);

    pstUser->LastSduCnt = 0;
    pstUser->State = 0;

    // Call other Actor to write. Conflict from different instances of User
    UserMgmt_RemoveUser1(pstUser->CellId, pstUser->InstanceId);

    // Write global variable. Conflict from different instances of User
    g_ReleaseCounter--;

    // Atomic operation. No conflict.
    SRE_AtomicAddU32((UINT32*)&g_ReleaseCounter, -1);

    L2OS_SpinLock(gCounterLock);
        g_ReleaseCounter--;
    L2OS_SpinUnLock(gCounterLock);
}


VOID User_Release2(VOID *pMsg) {
    MUM_ENTITY_STRU* pstUser = MUM_GetInstance(((USER_JOB_STRU*)pMsg)->ulInstanceId);

    pstUser->LastSduCnt = 0;
    pstUser->State = 0;

    // Call other Actor to write. No conflict due to the use of Lock in the callee
    UserMgmt_RemoveUser2(pstUser->CellId, pstUser->InstanceId);
}


VOID User_Release3(VOID *pMsg) {
    MUM_ENTITY_STRU* pstUser = (MUM_ENTITY_STRU*)(&gx_pstUserEntityTable[((USER_JOB_STRU*)pMsg)->ulInstanceId]);

    pstUser->LastSduCnt = 0;
    pstUser->State = 0;

    // Call other Actor to write. Deadlock
    UserMgmt_RemoveUser3(pstUser->CellId, pstUser->InstanceId);
}


VOID User_UpdateState(VOID *pMsg) {
    MUM_ENTITY_STRU* pstUser = MUM_GetInstance(((USER_JOB_STRU*)pMsg)->ulInstanceId);

    pstUser->State = 10;   // Conflict with User_UpdateCheckState
}


VOID User_UpdateCheckState(MUM_ENTITY_STRU *pstUser) {
    pstUser->CheckCount++; // Write data of other Actor, but no other Actor accesses such variable.

    if (pstUser->CheckCount == 500)
        pstUser->State = 5;   // Conflict with User_UpdateCheckState, other Actor may access this data.
}


/***************************************************
 *
 * MUCM Actor
 *
 **************************************************/

typedef struct
{
    UINT32 CellId;
} USER_MGMT_JOB_STRU;

typedef struct
{
    UINT32 CellId;
    UINT32 UserId;
} USER_MGMT_ADD_USER_JOB_STRU;


typedef struct {
    UINT32 CellId;
    SRE_LOCK_ID Lock;
    UINT32 UserCount;
    UINT32 UserList[128];
} USER_MGMT_STRU;
volatile USER_MGMT_STRU __l2_shared__ g_st_UserMgmtInstance[6];
volatile SRE_LOCK_ID __l2_shared__ g_UserMgmtGlobalLock;

typedef enum {
    USER_MGMT_TIMER = 0,
    USER_MGMT_ADD_USER,
} USER_MGMT_JOB_ENUM;


VOID UserMgmt_RemoveUser1(UINT32 cellId, UINT32 userId)
{
    int userCount = g_st_UserMgmtInstance[cellId].UserCount;

    for (int i = 0; i < userCount; i++)
    {
        if (g_st_UserMgmtInstance[cellId].UserList[i] // Conflict
            == userId)
        {
            // Conflict
            g_st_UserMgmtInstance[cellId].UserList[i] = g_st_UserMgmtInstance[cellId].UserList[userCount - 1];

            // Conflict
            g_st_UserMgmtInstance[cellId].UserCount--;
            break;
        }
    }
}


VOID UserMgmt_RemoveUser2(UINT32 cellId, UINT32 userId)
{
    L2OS_SpinLock(g_st_UserMgmtInstance[cellId].Lock);
    int userCount = g_st_UserMgmtInstance[cellId].UserCount;

    for (int i = 0; i < userCount; i++)
    {
        if (g_st_UserMgmtInstance[cellId].UserList[i] // No conflict
            == userId)
        {
            // No conflict
            g_st_UserMgmtInstance[cellId].UserList[i] = g_st_UserMgmtInstance[cellId].UserList[userCount - 1];

            // No conflict
            g_st_UserMgmtInstance[cellId].UserCount--;
            break;
        }
    }
    L2OS_SpinUnLock(g_st_UserMgmtInstance[cellId].Lock);
}


/**
 * Thus function uses only one shared lock
 */
VOID UserMgmt_RemoveUser3(UINT32 cellId, UINT32 userId)
{
    L2OS_SpinLock(g_UserMgmtGlobalLock);
    int userCount = g_st_UserMgmtInstance[cellId].UserCount;

    for (int i = 0; i < userCount; i++)
    {
        if (g_st_UserMgmtInstance[cellId].UserList[i] // No conflict
            == userId)
        {
            // No conflict
            g_st_UserMgmtInstance[cellId].UserList[i] = g_st_UserMgmtInstance[cellId].UserList[userCount - 1];

            // No conflict
            g_st_UserMgmtInstance[cellId].UserCount--;
            break;
        }
    }
    L2OS_SpinUnLock(g_UserMgmtGlobalLock);
}


/**
 * Concurrency error pattern 2
 * Read data
 */
void UserMgmt_OnTimer(void *pMsg) {
    USER_MGMT_JOB_STRU* pstJob = (USER_MGMT_JOB_STRU*)pMsg;

    for (int i = 0; i < g_st_UserMgmtInstance[pstJob->CellId].UserCount; i++)
    {
        MUM_ENTITY_STRU* pstInstance = MUM_GetInstance(g_st_UserMgmtInstance[pstJob->CellId].UserList[i]);
        if (pstInstance->State == VALID)      // Read data from other Actor
            User_UpdateCheckState(pstInstance);    // May have logic issues
    }

}

VOID UserMgmt_AddUser(VOID *pMsg) {
    USER_MGMT_ADD_USER_JOB_STRU* pstJob = (USER_MGMT_ADD_USER_JOB_STRU*)pMsg;

    volatile USER_MGMT_STRU* pstMucmInstrance = &g_st_UserMgmtInstance[pstJob->CellId];
    L2OS_SpinLock(g_UserMgmtGlobalLock);
        pstMucmInstrance->UserList[pstMucmInstrance->UserCount++] = pstJob->UserId;
    L2OS_SpinUnLock(g_UserMgmtGlobalLock);
}


/**
 * Actor's Job list
 */
JSF_JOB_REG_INFO __l2_shared__ g_stUserMgmtJobTable[] =
{
    {USER_MGMT_TIMER, UserMgmt_OnTimer},
    {USER_MGMT_ADD_USER, UserMgmt_AddUser},
};


int main(int argc, char *argv[]) {

    // Init g_st_UserMgmtInstance
    gx_pstUserEntityTable = (MUM_ENTITY_STRU*) malloc(sizeof(MUM_ENTITY_STRU));

    // Init SRE locks
    g_UserMgmtGlobalLock = SRE_LockCreate(SRE_SPINLOCK_TYPE);
    for (int i = 0; i < 6; ++i) {
        g_st_UserMgmtInstance[i].Lock = SRE_LockCreate(SRE_SPINLOCK_TYPE);
    }

    USER_MGMT_ADD_USER_JOB_STRU pstJob;

    // Create threads for individual scenarios
    pthread_t threads[16];

    //@{
    pthread_create(&threads[0], NULL, User_UpdateState, (VOID*) &pstJob);
    pthread_create(&threads[1], NULL, UserMgmt_OnTimer, (VOID*) &pstJob);
    //@}

//    //@{
//    pthread_create(&threads[0], NULL, User_Release1, (VOID*) &pstJob);
//    pthread_create(&threads[1], NULL, User_Release1, (VOID*) &pstJob);
//    //@}

//    //@{
//    pthread_create(&threads[0], NULL, User_Release2, (VOID*) &pstJob);
//    pthread_create(&threads[1], NULL, User_Release2, (VOID*) &pstJob);
//    //@}

//    //@{
//    pthread_create(&threads[0], NULL, User_Release3, (VOID*) &pstJob);
//    pthread_create(&threads[1], NULL, User_Release3, (VOID*) &pstJob);
//    //@}


    // Create threads for all pstJob
//    //@{
//    pthread_t threads_UserJob[16];
//    pthread_t threads_UserMgmtJob[16];
//    for (int i = 0; i < 4; ++i) {
//        void (*fn)(void*) = g_stUserJobTable[i].JSF_JOB_ENTRY_FUNC;
//        pthread_create(&threads_UserJob[i], NULL, fn, (VOID*) &pstJob);
//    }
//    for (int i = 0; i < 2; ++i) {
//        void (*fn)(void*) = g_stUserMgmtJobTable[i].JSF_JOB_ENTRY_FUNC;
//        pthread_create(&threads_UserMgmtJob[i], NULL, fn, (VOID*) &pstJob);
//    }
//    //@}

    return 0;
}
