#include "SelfTestEngine.h"
#include "StateMachine.h"
#include "CentrifugeTest.h"
#include "PressureTest.h"
#include <stdio.h>

// @see https://github.com/endurodave/C_StateMachine

BOOL DispatchCallbackThread1(const CB_CallbackMsg* cbMsg);

// Self test engine object structure
typedef struct
{
    BOOL testingActive;
} SelfTestEngine;

// Define public callback interfaces
CB_DEFINE(STE_StatusCb, const SelfTestStatus*, sizeof(SelfTestStatus), 1)
CB_DEFINE(STE_CompletedCb, void*, 0, 1)
CB_DEFINE(STE_FailedCb, void*, 0, 1)

// Define private instance of state machine
SelfTestEngine selfTestEngineObj;
SM_DEFINE(SelfTestEngineSM, &selfTestEngineObj)

// Private event
EVENT_DECLARE(STE_Complete, NoEventData)

// State enumeration order must match the order of state
// method entries in the state map
enum States
{
    ST_IDLE,
    ST_COMPLETED,
    ST_FAILED,
    ST_START_CENTRIFUGE_TEST,
    ST_START_PRESSURE_TEST,
    ST_MAX_STATES
};

// State machine state functions
STATE_DECLARE(Idle, NoEventData)
STATE_DECLARE(Completed, NoEventData)
STATE_DECLARE(Failed, NoEventData)
STATE_DECLARE(StartCentrifugeTest, NoEventData)
STATE_DECLARE(StartPressureTest, NoEventData)

// State map to define state function order
BEGIN_STATE_MAP(SelfTestEngine)
    STATE_MAP_ENTRY(ST_Idle)
    STATE_MAP_ENTRY(ST_Completed)
    STATE_MAP_ENTRY(ST_Failed)
    STATE_MAP_ENTRY(ST_StartCentrifugeTest)
    STATE_MAP_ENTRY(ST_StartPressureTest)
END_STATE_MAP(SelfTestEngine)

// Start external event
EVENT_DEFINE(STE_Start, NoEventData)
{
    // Given the Start event, transition to a new state based upon 
    // the current state of the state machine
    BEGIN_TRANSITION_MAP                                // - Current State -
        TRANSITION_MAP_ENTRY(ST_START_CENTRIFUGE_TEST)  // ST_Idle
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_Completed
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_Failed
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_StartCentrifugeTest
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_StartPressureTest
    END_TRANSITION_MAP(SelfTestEngine, pEventData)
}

// Complete external event
EVENT_DEFINE(STE_Complete, NoEventData)
{
    // Given the Complete event, transition to a new state based upon 
    // the current state of the state machine
    BEGIN_TRANSITION_MAP                                // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)             // ST_Idle
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_Completed
        TRANSITION_MAP_ENTRY(CANNOT_HAPPEN)             // ST_Failed
        TRANSITION_MAP_ENTRY(ST_START_PRESSURE_TEST)    // ST_StartCentrifugeTest
        TRANSITION_MAP_ENTRY(ST_COMPLETED)              // ST_StartPressureTest
    END_TRANSITION_MAP(SelfTestEngine, pEventData)
}

// Cancel external event
EVENT_DEFINE(STE_Cancel, NoEventData)
{
    // Given the Cancel event, transition to a new state based upon 
    // the current state of the state machine
    BEGIN_TRANSITION_MAP                        // - Current State -
        TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_Idle
        TRANSITION_MAP_ENTRY(ST_FAILED)         // ST_Completed
        TRANSITION_MAP_ENTRY(ST_FAILED)         // ST_Failed
        TRANSITION_MAP_ENTRY(ST_FAILED)         // ST_StartCentrifugeTest
        TRANSITION_MAP_ENTRY(ST_FAILED)         // ST_StartPressureTest
    END_TRANSITION_MAP(SelfTestEngine, pEventData)
}

static void STE_CompletedCallback(void* data, void* userData)
{
    SM_Event(SelfTestEngineSM, STE_Complete, NULL);
}

static void STE_FailedCallback(void* data, void* userData)
{
    SM_Event(SelfTestEngineSM, STE_Cancel, NULL);
}

void STE_Init()
{
    // Register with CentrifugeTest and PressureTest state machines
    CB_Register(CFG_CompletedCb, STE_CompletedCallback, DispatchCallbackThread1, NULL);
    CB_Register(CFG_FailedCb, STE_FailedCallback, DispatchCallbackThread1, NULL);
    CB_Register(PRE_CompletedCb, STE_CompletedCallback, DispatchCallbackThread1, NULL);
    CB_Register(PRE_FailedCb, STE_FailedCallback, DispatchCallbackThread1, NULL);
}

STATE_DEFINE(Idle, NoEventData)
{
    printf("%s ST_Idle\n", self->name);

    // Get pointer to the instance data 
    SelfTestEngine* pInstance = SM_GetInstance(SelfTestEngine);
    pInstance->testingActive = FALSE;

    // Asynchronously notify subscribers that testing status changed
    SelfTestStatus status;
    status.testActive = pInstance->testingActive;
    CB_Invoke(STE_StatusCb, &status);
}

STATE_DEFINE(Completed, NoEventData)
{
    printf("%s ST_Completed\n", self->name);

    // Asynchronously notify subscribers that testing completed
    CB_Invoke(STE_CompletedCb, NULL);

    // Transition to ST_Idle via an internal event
    SM_InternalEvent(ST_IDLE, NULL);
}

STATE_DEFINE(Failed, NoEventData)
{
    printf("%s ST_Failed\n", self->name);

    // Asynchronously notify subscribers that testing failed
    CB_Invoke(STE_FailedCb, NULL);

    // Transition to ST_Idle via an internal event
    SM_InternalEvent(ST_IDLE, NULL);
}

STATE_DEFINE(StartCentrifugeTest, NoEventData)
{
    printf("%s ST_StartCentrifugeTest\n", self->name);

    // Get pointer to the instance data 
    SelfTestEngine* pInstance = SM_GetInstance(SelfTestEngine);
    pInstance->testingActive = TRUE;

    // Start centrifuge test state machine
    SM_Event(CentrifugeTestSM, CFG_Start, NULL);

    // Asynchronously notify subscribers that testing status changed
    SelfTestStatus status;
    status.testActive = pInstance->testingActive;
    CB_Invoke(STE_StatusCb, &status);
}

STATE_DEFINE(StartPressureTest, NoEventData)
{
    printf("%s ST_StartPressureTest\n", self->name);

    // Start pressure test state machine 
    SM_Event(PressureTestSM, PRE_Start, NULL);
}

