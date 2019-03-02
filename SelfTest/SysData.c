#include "SysData.h"
#include "LockGuard.h"

// Define a public SysData callback interface
CB_DEFINE(SystemModeChangedCb, const SystemModeData*, sizeof(SystemModeData), 2)

// The current system mode data
static SystemModeType _systemMode;

// SysData lock handle
static LOCK_HANDLE _hLock;

//----------------------------------------------------------------------------
// SD_Init
//----------------------------------------------------------------------------
void SD_Init(void)
{
    _hLock = LK_CREATE();
}

//----------------------------------------------------------------------------
// SD_Term
//----------------------------------------------------------------------------
void SD_Term(void)
{
    LK_DESTROY(_hLock);
}

//----------------------------------------------------------------------------
// SD_SetSystemMode
//----------------------------------------------------------------------------
void SD_SetSystemMode(SystemModeType systemMode)
{
    LK_LOCK(_hLock);

    // Create the callback data
    SystemModeData callbackData;
    callbackData.PreviousSystemMode = _systemMode;
    callbackData.CurrentSystemMode = systemMode;

    // Update the system mode
    _systemMode = systemMode;

    // Callback all registered subscribers
    CB_Invoke(SystemModeChangedCb, &callbackData);

    LK_UNLOCK(_hLock);
}