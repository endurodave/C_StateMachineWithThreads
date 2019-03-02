#include "Timer.h"
#include "Fault.h"
#include "LockGuard.h"
#include <time.h>

typedef struct
{
    INT cbIdx;              
    DWORD timeout;	    	// in ticks
    DWORD expireTime;		// in ticks
    BOOL enabled;
} TMR_Obj;

// Maximum number of active timers
#define MAX_TIMERS  5

static TMR_Obj timerObjs[MAX_TIMERS];
static LOCK_HANDLE _hLock;

// Create async callback
CB_DECLARE(TMR_ExpiredCb, const void*)
CB_DEFINE(TMR_ExpiredCb, const void*, 0, MAX_TIMERS)

void TMR_Init(void)
{
    _hLock = LK_CREATE();
}

void TMR_Term(void)
{
    LK_DESTROY(_hLock);
}

BOOL TMR_Start(CB_CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc, DWORD timeout)
{
    BOOL success = FALSE;

    LK_LOCK(_hLock);

    BOOL registerSuccess = CB_Register(TMR_ExpiredCb, cbFunc, cbDispatchFunc, NULL);
    if (registerSuccess)
    {
        // Search for the registered data within the array
        for (size_t idx = 0; idx < MAX_TIMERS; idx++)
        {
            // Get pointer to an element within the multicast callback array 
            // created by CB_DEFINE macro
            CB_Info* info = &TMR_ExpiredCbMulticast[idx];

            // Does caller's callback match?
            if (info->cbFunc == cbFunc &&
                info->cbDispatchFunc == cbDispatchFunc)
            {
                // Ensure we are not overwriting a enabled timer
                ASSERT_TRUE(timerObjs[idx].enabled == FALSE);

                // Save timer data into array at same index as callback
                timerObjs[idx].timeout = timeout;
                timerObjs[idx].expireTime = timeout;
                timerObjs[idx].cbIdx = idx;
                timerObjs[idx].enabled = TRUE;
                success = TRUE;
                break;
            }
        }
    }

    LK_UNLOCK(_hLock);

    return success;
}

void TMR_Stop(CB_CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc)
{
    LK_LOCK(_hLock);

    // Search for the registered data within the array
    for (size_t idx = 0; idx < MAX_TIMERS; idx++)
    {
        // Get pointer to an element within the multicast callback array 
        // created by CB_DEFINE macro
        CB_Info* info = &TMR_ExpiredCbMulticast[idx];

        // Does caller's callback match?
        if (info->cbFunc == cbFunc &&
            info->cbDispatchFunc == cbDispatchFunc)
        {
            timerObjs[idx].enabled = FALSE;
            break;
        }
    }

    CB_Unregister(TMR_ExpiredCb, cbFunc, cbDispatchFunc);

    LK_UNLOCK(_hLock);
}

static DWORD TMR_Difference(DWORD time1, DWORD time2)
{
    return (time2 - time1);
}

static void TMR_CheckExpired(TMR_Obj* timer)
{
    ASSERT_TRUE(timer != NULL);

    if (!timer->enabled)
        return;

    // Has the timer expired?
    if (TMR_Difference(timer->expireTime, GetTickCount()) < timer->timeout)
        return;

    // Increment the timer to the next expiration
    timer->expireTime += timer->timeout;

    // Is the timer already expired after we incremented above?
    if (TMR_Difference(timer->expireTime, GetTickCount()) > timer->timeout)
    {
        // The timer has fallen behind so set time expiration further forward.
        timer->expireTime = GetTickCount();
    }

    // Ensure index is within range
    ASSERT_TRUE(timer->cbIdx < MAX_TIMERS);

    // Call the client's expired callback function asynchronously
    _CB_Dispatch(&TMR_ExpiredCbMulticast[timer->cbIdx], 1, NULL, 0);
}

void TMR_ProcessTimers()
{
    LK_LOCK(_hLock);

    // Iterate through each timer and check for expirations
    for (size_t idx = 0; idx < MAX_TIMERS; idx++)
    {
        TMR_CheckExpired(&timerObjs[idx]);
    }

    LK_UNLOCK(_hLock);
}
