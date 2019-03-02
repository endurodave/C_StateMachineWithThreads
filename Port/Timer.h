#ifndef _TIMER_H
#define _TIMER_H

#include "callback.h"

#ifdef __cplusplus
extern "C" {
#endif

void TMR_Init();
void TMR_Term();
BOOL TMR_Start(CB_CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc, DWORD timeout);
void TMR_Stop(CB_CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc);

// Called periodically by a thread to process all timers
void TMR_ProcessTimers();

#ifdef __cplusplus
}
#endif

#endif
