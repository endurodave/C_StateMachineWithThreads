#ifndef _CENTRIFUGE_TEST_H
#define _CENTRIFUGE_TEST_H

#include "DataTypes.h"
#include "StateMachine.h"
#include "callback.h"

#ifdef __cplusplus
extern "C" {
#endif

CB_DECLARE(CFG_CompletedCb, void*)
CB_DECLARE(CFG_FailedCb, void*)

// Declare the private instance of CentrifugeTest state machine
SM_DECLARE(CentrifugeTestSM)

// State machine event functions
EVENT_DECLARE(CFG_Start, NoEventData)
EVENT_DECLARE(CFG_Cancel, NoEventData)

#ifdef __cplusplus
}
#endif

#endif // _CENTRIFUGE_TEST_H
