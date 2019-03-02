#include "PressureTest.h"
#include "StateMachine.h"
#include "Timer.h"
#include <stdio.h>

BOOL DispatchCallbackThread1(const CB_CallbackMsg* cbMsg);

// CentrifugeTest object structure
typedef struct
{
    INT pressure;
} PressureTest;

// Define public callback interfaces
CB_DEFINE(PRE_CompletedCb, void*, 0, 1)
CB_DEFINE(PRE_FailedCb, void*, 0, 1)

// Define private instance of state machine
PressureTest pressureTestObj;
SM_DEFINE(PressureTestSM, &pressureTestObj)

// State enumeration order must match the order of state
// method entries in the state map
enum States
{
    ST_IDLE,
    ST_COMPLETED,
    ST_FAILED,
    ST_START_TEST,
    ST_MAX_STATES
};

// State machine state functions
STATE_DECLARE(Idle, NoEventData)
ENTRY_DECLARE(Idle, NoEventData)
STATE_DECLARE(Completed, NoEventData)
STATE_DECLARE(Failed, NoEventData)
STATE_DECLARE(StartTest, NoEventData)
GUARD_DECLARE(StartTest, NoEventData)

// State map to define state function order
BEGIN_STATE_MAP_EX(PressureTest)
    STATE_MAP_ENTRY_ALL_EX(ST_Idle, 0, EN_Idle, 0)
    STATE_MAP_ENTRY_EX(ST_Completed)
    STATE_MAP_ENTRY_EX(ST_Failed)
    STATE_MAP_ENTRY_ALL_EX(ST_StartTest, GD_StartTest, 0, 0)
END_STATE_MAP_EX(PressureTest)

EVENT_DEFINE(PRE_Start, NoEventData)
{
    BEGIN_TRANSITION_MAP                                // - Current State -
        TRANSITION_MAP_ENTRY(ST_START_TEST)             // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_COMPLETED
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_FAILED
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_START_TEST
    END_TRANSITION_MAP(PressureTest, pEventData)
}

EVENT_DEFINE(PRE_Cancel, NoEventData)
{
    BEGIN_TRANSITION_MAP                                // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_COMPLETED
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_FAILED
        TRANSITION_MAP_ENTRY(ST_FAILED)                 // ST_START_TEST
    END_TRANSITION_MAP(PressureTest, pEventData)
}

EVENT_DEFINE(PRE_Poll, NoEventData)
{
    BEGIN_TRANSITION_MAP                                    // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_IDLE
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_COMPLETED
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_FAILED
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_START_TEST
    END_TRANSITION_MAP(PressureTest, pEventData)
}

static void PRE_PollCallback(const void* data, void* userData)
{
    SM_Event(PressureTestSM, PRE_Poll, NULL);
}

STATE_DEFINE(Idle, NoEventData)
{
    printf("%s ST_Idle\n", self->name);
}

ENTRY_DEFINE(Idle, NoEventData)
{
    printf("%s EN_Idle\n", self->name);
    pressureTestObj.pressure = 0;

    // Stop timer callbacks
    TMR_Stop(PRE_PollCallback, DispatchCallbackThread1);
}

STATE_DEFINE(Completed, NoEventData)
{
    printf("%s ST_Completed\n", self->name);
    SM_InternalEvent(ST_IDLE, NULL);

    // Asynchronously notify subscribers that testing completed
    CB_Invoke(PRE_CompletedCb, NULL);
}

STATE_DEFINE(Failed, NoEventData)
{
    printf("%s ST_Failed\n", self->name);
    SM_InternalEvent(ST_IDLE, NULL);

    // Asynchronously notify subscribers that testing failed
    CB_Invoke(PRE_FailedCb, NULL);
}

// Start the centrifuge test state.
STATE_DEFINE(StartTest, NoEventData)
{
    printf("%s ST_StartTest\n", self->name);

    SM_InternalEvent(ST_COMPLETED, NULL);
}

// Guard condition to determine whether StartTest state is executed.
GUARD_DEFINE(StartTest, NoEventData)
{
    printf("%s GD_StartTest\n", self->name);
    if (pressureTestObj.pressure == 0)
        return TRUE;    // Pressure low. OK to start test.
    else
        return FALSE;   // Pressure high. Can't start test.
}




