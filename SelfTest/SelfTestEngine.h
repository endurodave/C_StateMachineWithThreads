#ifndef _SELF_TEST_ENGINE_H
#define _SELF_TEST_ENGINE_H

#include "DataTypes.h"
#include "StateMachine.h"
#include "callback.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    BOOL testActive;
} SelfTestStatus;

// Declare public callback interfaces
CB_DECLARE(STE_StatusCb, const SelfTestStatus*)
CB_DECLARE(STE_CompletedCb, void*)
CB_DECLARE(STE_FailedCb, void*)

// Declare the private instance of SelfTestEngine state machine
SM_DECLARE(SelfTestEngineSM)

// State machine event functions
EVENT_DECLARE(STE_Start, NoEventData)
EVENT_DECLARE(STE_Cancel, NoEventData)

void STE_Init();

#ifdef __cplusplus
}
#endif

#endif // _SELF_TEST_ENGINE_H
