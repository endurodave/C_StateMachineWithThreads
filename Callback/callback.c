#include "callback.h"
#include "DataTypes.h"
#include "Fault.h"
#include <string.h>

// Define USE_LOCK to use the default lock implementation
#define USE_LOCKS
#ifdef USE_LOCKS
    #include "LockGuard.h"
    static LOCK_HANDLE _hLock;
#else
    #pragma message("WARNING: Define software lock.")
    typedef int LOCK_HANDLE;
    static LOCK_HANDLE _hLock;

    #define LK_CREATE()     (1)
    #define LK_DESTROY(h)  
    #define LK_LOCK(h)    
    #define LK_UNLOCK(h)  
#endif

// Define USE_CALLBACK_ALLOCATOR to use the fixed block allocator instead of heap
#define USE_CALLBACK_ALLOCATOR
#ifdef USE_CALLBACK_ALLOCATOR
    #define XALLOC(size)    CBALLOC_Alloc(size)
    #define XFREE(ptr)      CBALLOC_Free(ptr)
#else
    #include <stdlib.h>
    #define XALLOC(size)    malloc(size)
    #define XFREE(ptr)      free(ptr)
#endif

static BOOL CB_DispatchCallback(const CB_Info* cbInfo, const void* cbData, size_t cbDataSize);

//----------------------------------------------------------------------------
// CB_DispatchCallback
//----------------------------------------------------------------------------
static BOOL CB_DispatchCallback(const CB_Info* cbInfo, const void* cbData, size_t cbDataSize)
{
    BOOL success = FALSE;
    BOOL dispatchSuccess = FALSE;
    CB_CallbackMsg* cbMsg = NULL;
    void* cbDataCopy = NULL;

    ASSERT_TRUE(cbInfo);

    // Is an OS task dispatch function defined? 
    if (cbInfo->cbDispatchFunc == NULL)
    {
        ASSERT_TRUE(cbInfo->cbFunc);

        // No OS task dispatch function. Synchronously invoke callback function.
        cbInfo->cbFunc(cbData, cbInfo->cbUserData);
        return TRUE;
    }

    // Is there callback data?
    if (cbDataSize > 0)
    {
        // Allocate fixed block memory for callback argument data
        cbDataCopy = XALLOC(cbDataSize);
    }

    // Allocate fixed block memory for a callback message
    cbMsg = (CB_CallbackMsg*)XALLOC(sizeof(CB_CallbackMsg));
    if (cbMsg)
    {
        if (cbDataCopy)
        {
            // Bitwise copy callback data argument
            memcpy(cbDataCopy, cbData, cbDataSize);
        }

        // Copy callback function and argument data pointers into callback message
        cbMsg->cbFunc = cbInfo->cbFunc;
        cbMsg->cbData = cbDataCopy;
        cbMsg->cbUserData = cbInfo->cbUserData;

        // Dispatch the callback message onto the OS task
        dispatchSuccess = cbInfo->cbDispatchFunc(cbMsg);

        // Did the message get dispatched onto the OS task?
        if (dispatchSuccess)
        {
            // Success! Callback dispatched to target task.
            success = TRUE;
        }
    }
    else
    {
        CBALLOC_Free(cbMsg);
        CBALLOC_Free(cbDataCopy);

        // Out of memory
        ASSERT();
    }

    return success;
} 

//----------------------------------------------------------------------------
// CB_Init
//----------------------------------------------------------------------------
void CB_Init(void)
{
    _hLock = LK_CREATE();
}

//----------------------------------------------------------------------------
// CB_Term
//----------------------------------------------------------------------------
void CB_Term(void)
{
    LK_DESTROY(_hLock);
}

//----------------------------------------------------------------------------
// CB_TargetInvoke
//----------------------------------------------------------------------------
void CB_TargetInvoke(const CB_CallbackMsg* cbMsg)
{
    ASSERT_TRUE(cbMsg);
    ASSERT_TRUE(cbMsg->cbFunc);

    // Invoke callback function with the callback data
    cbMsg->cbFunc(cbMsg->cbData, cbMsg->cbUserData);

    // Free data sent through OS queue
    XFREE((void*)cbMsg->cbData);
    XFREE((void*)cbMsg);
}

//----------------------------------------------------------------------------
// _CB_AddCallback
//----------------------------------------------------------------------------
BOOL _CB_AddCallback(CB_Info* cbInfo,
    size_t cbInfoLen,
    CB_CallbackFuncType cbFunc,
    CB_DispatchCallbackFuncType cbDispatchFunc,
    void* cbUserData)
{
    BOOL success = FALSE;

    ASSERT_TRUE(cbInfo);
    ASSERT_TRUE(cbInfoLen > 0);
    ASSERT_TRUE(cbFunc);

    LK_LOCK(_hLock);

    // Search for an empty registration within the callback array
    for (size_t idx = 0; idx<cbInfoLen; idx++)
    {
        // Empty registration slot?
        if (cbInfo[idx].cbFunc == NULL)
        {
            // Save callback information into cbInfo array
            cbInfo[idx].cbFunc = cbFunc;
            cbInfo[idx].cbDispatchFunc = cbDispatchFunc;
            cbInfo[idx].cbUserData = cbUserData;
            success = TRUE;
            break;
        }
    }

    LK_UNLOCK(_hLock);

    // Assert if all registration locations are full
    ASSERT_TRUE(success == TRUE);
    return success;
} 

//----------------------------------------------------------------------------
// _CB_RemoveCallback
//----------------------------------------------------------------------------
BOOL _CB_RemoveCallback(CB_Info* cbInfo,
    size_t cbInfoLen,
    CB_CallbackFuncType cbFunc,
    CB_DispatchCallbackFuncType cbDispatchFunc)
{
    BOOL success = FALSE;

    ASSERT_TRUE(cbInfo);
    ASSERT_TRUE(cbFunc);
    ASSERT_TRUE(cbInfoLen > 0);

    LK_LOCK(_hLock);

    // Search for the registered data within the callback array
    for (size_t idx = 0; idx<cbInfoLen; idx++)
    {
        // Does caller's callback match?
        if (cbInfo[idx].cbFunc == cbFunc &&
            cbInfo[idx].cbDispatchFunc == cbDispatchFunc)
        {
            // Remove callback function pointer from cbInfo array
            cbInfo[idx].cbFunc = NULL;
            cbInfo[idx].cbDispatchFunc = NULL;
            cbInfo[idx].cbUserData = NULL;
            success = TRUE;
            break;
        }
    }

    LK_UNLOCK(_hLock);
    return success;
} 

//----------------------------------------------------------------------------
// _CB_IsAdded
//----------------------------------------------------------------------------
BOOL _CB_IsAdded(CB_Info* cbInfo,
    size_t cbInfoLen,
    CB_CallbackFuncType cbFunc,
    CB_DispatchCallbackFuncType cbDispatchFunc)
{
    BOOL isAdded = FALSE;

    ASSERT_TRUE(cbInfo);
    ASSERT_TRUE(cbInfoLen > 0);
    ASSERT_TRUE(cbFunc);

    LK_LOCK(_hLock);

    // Search for the registered data within the callback array
    for (size_t idx = 0; idx<cbInfoLen; idx++)
    {
        // Does the caller's callback match?
        if (cbInfo[idx].cbFunc == cbFunc &&
            cbInfo[idx].cbDispatchFunc == cbDispatchFunc)
        {
            isAdded = TRUE;
            break;
        }
    }

    LK_UNLOCK(_hLock);
    return isAdded;
}

//----------------------------------------------------------------------------
// _CB_Dispatch
//----------------------------------------------------------------------------
BOOL _CB_Dispatch(const CB_Info* cbInfo, size_t cbInfoLen, const void* cbData, 
    size_t cbDataSize)
{
    BOOL invoked = FALSE;

    LK_LOCK(_hLock);

    // For each CB_Info instance within the array
    for (size_t idx = 0; idx<cbInfoLen; idx++)
    {
        // Is a client registered?
        if (cbInfo[idx].cbFunc)
        {
            // Dispatch callback onto the OS task
            if (CB_DispatchCallback(&cbInfo[idx], cbData, cbDataSize))
            {
                invoked = TRUE;
            }
        }
    }

    LK_UNLOCK(_hLock);
    return invoked;
}

