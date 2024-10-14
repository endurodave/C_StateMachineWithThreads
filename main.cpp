#include "SelfTestEngine.h"
#include "WorkerThreadStd.h"
#include "Timer.h"
#include "fb_allocator.h"

// @see https://github.com/endurodave/C_StateMachineWithThreads
// David Lafreniere

using namespace std;

// Simple flag to exit main loop
BOOL selfTestEngineCompleted = FALSE;

// Status callback from SelfTestEngine
void STE_StatusCallback(const SelfTestStatus* status, void* userData)
{
    printf("STE_StatusCallback %d\n", status->testActive);
}

// Completed callback from SelfTestEngine
void STE_CompletedCallback(void* data, void* userData)
{
    printf("STE_CompletedCallback\n");
    selfTestEngineCompleted = TRUE;
}

// Failed callback from SelfTestEngine
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

    // Register for SelfTestEngine callbacks on DispatchCallbackThread2
    CB_Register(STE_StatusCb, STE_StatusCallback, DispatchCallbackThread2, NULL);
    CB_Register(STE_CompletedCb, STE_CompletedCallback, DispatchCallbackThread2, NULL);
    CB_Register(STE_FailedCb, STE_FailedCallback, DispatchCallbackThread2, NULL);

    // Start SelfTestEngine
    SM_Event(SelfTestEngineSM, STE_Start, NULL);

    // Wait for SelfTestEngine to complete 
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

