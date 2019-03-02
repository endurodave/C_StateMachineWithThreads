#include "Timer.h"
#include "Fault.h"

typedef struct
{
    INT cbIdx;              
    DWORD timeout;	    	// in ticks
    DWORD expireTime;		// in ticks
    BOOL enabled;
} TMR_Obj;

#define MAX_TIMERS  5

static TMR_Obj timerObjs[MAX_TIMERS];
static LOCK_HANDLE _hLock;

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

    BOOL registerSuccess = CB_Register(TMR_ExpiredCb, cbFunc, cbDispatchFunc, NULL);

    LK_LOCK(_hLock);

    if (registerSuccess)
    {
        // Search for the registered data within the array
        for (size_t idx = 0; idx < MAX_TIMERS; idx++)
        {
            CB_Info* info = &TMR_ExpiredCbMulticast[idx];

            // Does caller's callback match?
            if (info->cbFunc == cbFunc &&
                info->cbDispatchFunc == cbDispatchFunc)
            {
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
    CB_Unregister(TMR_ExpiredCb, cbFunc, cbDispatchFunc);

    LK_LOCK(_hLock);

    // Search for the registered data within the array
    for (size_t idx = 0; idx < MAX_TIMERS; idx++)
    {
        CB_Info* info = &TMR_ExpiredCbMulticast[idx];

        // Does caller's callback match?
        if (info->cbFunc == cbFunc &&
            info->cbDispatchFunc == cbDispatchFunc)
        {
            timerObjs[idx].enabled = FALSE;
            break;
        }
    }

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

#if 0
using namespace std;

LOCK Timer::m_lock;
BOOL Timer::m_lockInit = FALSE;
BOOL Timer::m_timerStopped = FALSE;
list<Timer*> Timer::m_timers;
const DWORD Timer::MS_PER_TICK = (1000 / CLOCKS_PER_SEC);

//------------------------------------------------------------------------------
// TimerDisabled
//------------------------------------------------------------------------------
static bool TimerDisabled (Timer* value)
{
	return !(value->Enabled());
}

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
Timer::Timer() 
{
	// Create the thread mutex
	if (m_lockInit == FALSE)
	{
		LockGuard::Create(&m_lock);
		m_lockInit = TRUE;
	}

	LockGuard lockGuard(&m_lock);
	m_enabled = FALSE;
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
Timer::~Timer()
{
	LockGuard lockGuard(&m_lock);
	m_timers.remove(this);
}

//------------------------------------------------------------------------------
// Start
//------------------------------------------------------------------------------
void Timer::Start(DWORD timeout)
{
	LockGuard lockGuard(&m_lock);

	m_timeout = timeout / MS_PER_TICK;
    ASSERT_TRUE(m_timeout != 0);
	m_expireTime = GetTickCount();
	m_enabled = TRUE;

	// Remove the existing entry, if any, to prevent duplicates in the list
	m_timers.remove(this);

	// Add this timer to the list for servicing
	m_timers.push_back(this);
}

//------------------------------------------------------------------------------
// Stop
//------------------------------------------------------------------------------
void Timer::Stop()
{
	LockGuard lockGuard(&m_lock);

	m_enabled = FALSE;
	m_timerStopped = TRUE;
}

//------------------------------------------------------------------------------
// CheckExpired
//------------------------------------------------------------------------------
void Timer::CheckExpired()
{
	if (!m_enabled)
		return;

	// Has the timer expired?
    if (Difference(m_expireTime, GetTickCount()) < m_timeout)
        return;

    // Increment the timer to the next expiration
	m_expireTime += m_timeout;

	// Is the timer already expired after we incremented above?
    if (Difference(m_expireTime, GetTickCount()) > m_timeout)
	{
		// The timer has fallen behind so set time expiration further forward.
		m_expireTime = GetTickCount();
	}

	// Call the client's expired callback function
	if (Expired)
		Expired(NoData());
}

//------------------------------------------------------------------------------
// Difference
//------------------------------------------------------------------------------
DWORD Timer::Difference(DWORD time1, DWORD time2)
{
	return (time2 - time1);
}

//------------------------------------------------------------------------------
// ProcessTimers
//------------------------------------------------------------------------------
void Timer::ProcessTimers()
{
	LockGuard lockGuard(&m_lock);

	// Remove disabled timer from the list if stopped
	if (m_timerStopped)
	{
		m_timers.remove_if(TimerDisabled);
		m_timerStopped = FALSE;
	}

	// Iterate through each timer and check for expirations
	TimersIterator it;
	for (it = m_timers.begin() ; it != m_timers.end(); it++ )
	{
		if ((*it) != NULL)
			(*it)->CheckExpired();
	}
}
#endif

