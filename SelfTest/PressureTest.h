#ifndef _PRESSURE_TEST_H
#define _PRESSURE_TEST_H

#include "DataTypes.h"
#include "StateMachine.h"
#include "callback.h"

#ifdef __cplusplus
extern "C" {
#endif

CB_DECLARE(PRE_CompletedCb, void*)
CB_DECLARE(PRE_FailedCb, void*)

// Declare the private instance of PressureTest state machine
SM_DECLARE(PressureTestSM)

// State machine event functions
EVENT_DECLARE(PRE_Start, NoEventData)
EVENT_DECLARE(PRE_Cancel, NoEventData)

#ifdef __cplusplus
}
#endif

#endif // _PRESSURE_TEST_H
