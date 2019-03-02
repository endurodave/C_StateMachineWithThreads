#include "CentrifugeTest.h"
#include "StateMachine.h"
#include "Timer.h"
#include <stdio.h>

BOOL DispatchCallbackThread1(const CB_CallbackMsg* cbMsg);

// CentrifugeTest object structure
typedef struct
{
    INT speed;
} CentrifugeTest;

// Define public callback interfaces
CB_DEFINE(CFG_CompletedCb, void*, 0, 1)
CB_DEFINE(CFG_FailedCb, void*, 0, 1)

// Define private instance of state machine
CentrifugeTest centrifugeTestObj;
SM_DEFINE(CentrifugeTestSM, &centrifugeTestObj)

// State enumeration order must match the order of state
// method entries in the state map
enum States
{
    ST_IDLE,
    ST_COMPLETED,
    ST_FAILED,
    ST_START_TEST,
    ST_ACCELERATION,
    ST_WAIT_FOR_ACCELERATION,
    ST_DECELERATION,
    ST_WAIT_FOR_DECELERATION,
    ST_MAX_STATES
};

// State machine state functions
STATE_DECLARE(Idle, NoEventData)
ENTRY_DECLARE(Idle, NoEventData)
STATE_DECLARE(Completed, NoEventData)
STATE_DECLARE(Failed, NoEventData)
STATE_DECLARE(StartTest, NoEventData)
GUARD_DECLARE(StartTest, NoEventData)
STATE_DECLARE(Acceleration, NoEventData)
STATE_DECLARE(WaitForAcceleration, NoEventData)
EXIT_DECLARE(WaitForAcceleration)
STATE_DECLARE(Deceleration, NoEventData)
STATE_DECLARE(WaitForDeceleration, NoEventData)
EXIT_DECLARE(WaitForDeceleration)

// State map to define state function order
BEGIN_STATE_MAP_EX(CentrifugeTest)
    STATE_MAP_ENTRY_ALL_EX(ST_Idle, 0, EN_Idle, 0)
    STATE_MAP_ENTRY_EX(ST_Completed)
    STATE_MAP_ENTRY_EX(ST_Failed)
    STATE_MAP_ENTRY_ALL_EX(ST_StartTest, GD_StartTest, 0, 0)
    STATE_MAP_ENTRY_EX(ST_Acceleration)
    STATE_MAP_ENTRY_ALL_EX(ST_WaitForAcceleration, 0, 0, EX_WaitForAcceleration)
    STATE_MAP_ENTRY_EX(ST_Deceleration)
    STATE_MAP_ENTRY_ALL_EX(ST_WaitForDeceleration, 0, 0, EX_WaitForDeceleration)
END_STATE_MAP_EX(CentrifugeTest)

EVENT_DEFINE(CFG_Start, NoEventData)
{
    BEGIN_TRANSITION_MAP                                // - Current State -
        TRANSITION_MAP_ENTRY(ST_START_TEST)             // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_COMPLETED
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_FAILED
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_START_TEST
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_ACCELERATION
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_WAIT_FOR_ACCELERATION
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_DECELERATION
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_WAIT_FOR_DECELERATION
    END_TRANSITION_MAP(CentrifugeTest, pEventData)
}

EVENT_DEFINE(CFG_Cancel, NoEventData)
{
    BEGIN_TRANSITION_MAP                                // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_IDLE
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_COMPLETED
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_FAILED
        TRANSITION_MAP_ENTRY(ST_FAILED)                 // ST_START_TEST
        TRANSITION_MAP_ENTRY(ST_FAILED)                 // ST_ACCELERATION
        TRANSITION_MAP_ENTRY(ST_FAILED)                 // ST_WAIT_FOR_ACCELERATION
        TRANSITION_MAP_ENTRY(ST_FAILED)                 // ST_DECELERATION
        TRANSITION_MAP_ENTRY(ST_FAILED)                 // ST_WAIT_FOR_DECELERATION
    END_TRANSITION_MAP(CentrifugeTest, pEventData)
}

EVENT_DEFINE(CFG_Poll, NoEventData)
{
    BEGIN_TRANSITION_MAP                                    // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_IDLE
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_COMPLETED
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_FAILED
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)                 // ST_START_TEST
        TRANSITION_MAP_ENTRY(ST_WAIT_FOR_ACCELERATION)      // ST_ACCELERATION
        TRANSITION_MAP_ENTRY(ST_WAIT_FOR_ACCELERATION)      // ST_WAIT_FOR_ACCELERATION
        TRANSITION_MAP_ENTRY(ST_WAIT_FOR_DECELERATION)      // ST_DECELERATION
        TRANSITION_MAP_ENTRY(ST_WAIT_FOR_DECELERATION)      // ST_WAIT_FOR_DECELERATION
    END_TRANSITION_MAP(CentrifugeTest, pEventData)
}

static void CFG_PollCallback(const void* data, void* userData)
{
    SM_Event(CentrifugeTestSM, CFG_Poll, NULL);
}

STATE_DEFINE(Idle, NoEventData)
{
    printf("%s ST_Idle\n", self->name);
}

ENTRY_DEFINE(Idle, NoEventData)
{
    printf("%s EN_Idle\n", self->name);
    centrifugeTestObj.speed = 0;

    // Stop timer callbacks
    TMR_Stop(CFG_PollCallback, DispatchCallbackThread1);
}

STATE_DEFINE(Completed, NoEventData)
{
    printf("%s ST_Completed\n", self->name);
    SM_InternalEvent(ST_IDLE, NULL);

    // Asynchronously notify subscribers that testing completed
    CB_Invoke(CFG_CompletedCb, NULL);
}

STATE_DEFINE(Failed, NoEventData)
{
    printf("%s ST_Failed\n", self->name);
    SM_InternalEvent(ST_IDLE, NULL);

    // Asynchronously notify subscribers that testing failed
    CB_Invoke(CFG_FailedCb, NULL);
}

// Start the centrifuge test state.
STATE_DEFINE(StartTest, NoEventData)
{
    printf("%s ST_StartTest\n", self->name);
    SM_InternalEvent(ST_ACCELERATION, NULL);
}

// Guard condition to determine whether StartTest state is executed.
GUARD_DEFINE(StartTest, NoEventData)
{
    printf("%s GD_StartTest\n", self->name);
    if (centrifugeTestObj.speed == 0)
        return TRUE;    // Centrifuge stopped. OK to start test.
    else
        return FALSE;   // Centrifuge spinning. Can't start test.
}

// Start accelerating the centrifuge.
STATE_DEFINE(Acceleration, NoEventData)
{
    printf("%s ST_Acceleration\n", self->name);

    // Start polling while waiting for centrifuge to ramp up to speed
    TMR_Start(CFG_PollCallback, DispatchCallbackThread1, 100);
}

// Wait in this state until target centrifuge speed is reached.
STATE_DEFINE(WaitForAcceleration, NoEventData)
{
    printf("%s ST_WaitForAcceleration : Speed is %d\n", self->name, centrifugeTestObj.speed);
    if (++centrifugeTestObj.speed >= 5)
        SM_InternalEvent(ST_DECELERATION, NULL);
}

// Exit action when WaitForAcceleration state exits.
EXIT_DEFINE(WaitForAcceleration)
{
    printf("%s EX_WaitForAcceleration\n", self->name);

    // Acceleration over, stop timer polling
    TMR_Stop(CFG_PollCallback, DispatchCallbackThread1);
}

// Start decelerating the centrifuge.
STATE_DEFINE(Deceleration, NoEventData)
{
    printf("%s ST_Deceleration\n", self->name);

    // Start polling while waiting for centrifuge to ramp down to 0
    TMR_Start(CFG_PollCallback, DispatchCallbackThread1, 100);
}

// Wait in this state until centrifuge speed is 0.
STATE_DEFINE(WaitForDeceleration, NoEventData)
{
    printf("%s ST_WaitForDeceleration : Speed is %d\n", self->name, centrifugeTestObj.speed);
    if (centrifugeTestObj.speed-- == 0)
        SM_InternalEvent(ST_COMPLETED, NULL);
}

// Exit action when WaitForDeceleration state exits.
EXIT_DEFINE(WaitForDeceleration)
{
    printf("%s EX_WaitForDeceleration\n", self->name);

    // Deceleration over, stop timer polling
    TMR_Stop(CFG_PollCallback, DispatchCallbackThread1);
}


