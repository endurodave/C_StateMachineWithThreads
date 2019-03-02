#include "stdafx.h"
#include "callback.h"
#include "SelfTestEngine.h"
#include "WorkerThreadStd.h"
#include "Timer.h"
#include "fb_allocator.h"
#include <iostream>
#include <string.h>

using namespace std;

// Simple flag to exit main loop
BOOL selfTestEngineCompleted = FALSE;

void STE_StatusCallback(const SelfTestStatus* status, void* userData)
{
    // Output status message to the console "user interface"
    printf("STE_StatusCallback %d\n", status->testActive);
}

void STE_CompletedCallback(void* data, void* userData)
{
    printf("STE_CompletedCallback\n");
    selfTestEngineCompleted = TRUE;
}

void STE_FailedCallback(void* data, void* userData)
{
    printf("STE_FailedCallback\n");
    selfTestEngineCompleted = TRUE;
}

int main()
{
    // Initialize modules
    ALLOC_Init();
    TMR_Init();
	CB_Init();
    STE_Init();
    CreateThreads();

    // Register for self test engine callbacks on DispatchCallbackThread2
    CB_Register(STE_StatusCb, STE_StatusCallback, DispatchCallbackThread2, NULL);
    CB_Register(STE_CompletedCb, STE_CompletedCallback, DispatchCallbackThread2, NULL);
    CB_Register(STE_FailedCb, STE_FailedCallback, DispatchCallbackThread2, NULL);

    // Start self-test engine
    SM_Event(SelfTestEngineSM, STE_Start, NULL);

    // Wait for self-test engine to complete 
    while (!selfTestEngineCompleted)
        this_thread::sleep_for(1s);

    // Cleanup before exit
    ExitThreads();
    CB_Term();
    TMR_Term();
    ALLOC_Term();

    this_thread::sleep_for(1s);

    return 0;
}

