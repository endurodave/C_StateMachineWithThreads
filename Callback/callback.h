// https://www.codeproject.com/Articles/1272894/Asynchronous-Multicast-Callbacks-in-C
//
// The callback module implements synchronous or asynchronous function callbacks.
// A publisher exposes a callback interface using CB_DECLARE and CB_DEFINE macros. 
// A subscriber registers to receive callbacks at runtime using CB_Register().
//
// All callback functions must return void, have one pointer function argument
// and one user data void* argument. e.g. void MyCallback(MyData* arg, void* userData)
// Synchronous callbacks are invoked on the current executing task. Asynchronous
// callbacks are dispatched to the target OS task to be invoked. Callback data 
// is bitwise copied for transport through a message queue. Dynamic storage is
// obtained from the global heap or a fixed block memory allocator. 
//
// Each target OS task must implement a single function conforming to 
// CB_DispatchCallbackFuncType. The function implementation must post the pointer
// to CB_CallbackMsg into a message queue and call CB_TargetInvoke() on the 
// destination task. All dynamic storage allocation and deallocation is handled 
// automatically by the callback module. Abstracting the OS task and queue 
// implementation details makes the callback module generic to any application. 
//
// Publisher example:
//
// // CB_DECLARE typically placed in header file
// CB_DECLARE(TestCb, int*)  
//
// // CB_DEFINE placed in source file
// CB_DEFINE(TestCb, int*, sizeof(int), MAX_REGISTER)
//
// // Publisher invokes all registered callback functions
// int data = 123;
// CB_Invoke(TestCb, &data);
//
// Subscriber example:
// 
// // Callback function
// void TestCallback(int* data, void* userData)
// {
//    printf("My data: %d", *arg);
// }
//
// // Register to receive synchronous callbacks on TestCallback() function
// CB_Register(TestCb, TestCallback, NULL, NULL);
//
// // Register to receive asychronous callbacks on thread 1
// CB_Register(TestCb, TestCallback, DispatchCallbackThread1, NULL);
//
// // Unregister from publisher callbacks
// CB_Unregister(TestCb, TestCallback, NULL);
// CB_Unregister(TestCb, TestCallback, DispatchCallbackThread1);

#ifndef _CALLBACK_H
#define _CALLBACK_H

#include "callback_allocator.h"
#include "callback_allocator.h"
#include "DataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Callback function pointer type
typedef void (*CB_CallbackFuncType)(const void* cbData, void* cbUserData);

typedef struct 
{
    // A pointer to the registered callback function
    CB_CallbackFuncType cbFunc;

    // A pointer to the callback function data argument
    const void* cbData;

    // Optional user data passed back on each callback
    void* cbUserData;
} CB_CallbackMsg;

// Each OS task dispatch function must conform to this signature 
typedef BOOL (*CB_DispatchCallbackFuncType)(const CB_CallbackMsg* cbMsg);

typedef struct
{
    // A pointer to the registered callback function
    CB_CallbackFuncType cbFunc;

    // A pointer to a dispatch function that places CB_CallbackMsg into a message queue
    CB_DispatchCallbackFuncType cbDispatchFunc;

    // Optional user data passed back on each callback
    void* cbUserData;
} CB_Info;

// User macros to ease using the callback wrapper functions.
// cbName - the callback name as set within CB_DECLARE
// cbFunc - a callback function matching the callback signature
// cbDispatchFunc - the destination task dispatch function for an asynchronous 
//      callback or NULL for a synchronous callback
// cbArg - the callback function argument (must be a pointer type)
// cbNum - number of cbData elements pointed to by cbData
// cbSize - the size of each cbData element
// cbUserData - optional data passed back during each callback. Can point to 
//      anything the subscriber wants. Set to NULL if not using user data. 
// e.g. CB_Register(MyCallback, TestCallbackFunc, DispatchFunc);
#define CB_Register(cbName, cbFunc, cbDispatchFunc, cbUserData)  cbName##_Register(cbFunc, cbDispatchFunc, cbUserData)
#define CB_Unregister(cbName, cbFunc, cbDispatchFunc)            cbName##_Unregister(cbFunc, cbDispatchFunc)
#define CB_Invoke(cbName, cbArg)                                 cbName##_Invoke(cbArg)
#define CB_InvokeArray(cbName, cbArg, cbNum, cbSize)             cbName##_InvokeArray(cbArg, cbNum, cbSize)
#define CB_IsRegistered(cbName, cbFunc, cbDispatchFunc)          cbName##_IsRegistered(cbFunc, cbDispatchFunc)
#define CB_GetCbInfo(cbName, cbIdx)                              cbName##_GetCbInfo(cbIdx)

// Declare type-safe callback wrapper functions.
// cbName - name your callback with any unique name
// cbArg - the callback argument type. Must be a pointer type (e.g. int* or const MyData*)
// e.g. CB_DECLARE(MyCallback, int*)
#define CB_DECLARE(cbName, cbArg) \
    typedef void(*cbName##CallbackFuncType)(cbArg cbData, void* cbUserData); \
    BOOL cbName##_Register(cbName##CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc, void* cbUserData); \
    BOOL cbName##_IsRegistered(cbName##CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc); \
    BOOL cbName##_Unregister(cbName##CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc); \
    BOOL cbName##_Invoke(cbArg cbData); \
    BOOL cbName##_InvokeArray(cbArg cbData, size_t num, size_t size); \
    const CB_Info* cbName##_GetCbInfo(unsigned int cbIdx);

// Define type-safe callback wrapper functions.
// cbName - name your callback with any unique name
// cbArg - the callback argument type. Must be a pointer type. (e.g. int* or const MyData*)
// cbArgSize - size of the data pointed to by cbArg
// cbMax - the maximum allowed registered callbacks 
// e.g. CB_DEFINE(MyCallback, int*, sizeof(int), 2)
#define CB_DEFINE(cbName, cbArg, cbArgSize, cbMax) \
    static CB_Info cbName##Multicast[cbMax]; \
    BOOL cbName##_Register(cbName##CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc, void* cbUserData) { \
        return _CB_AddCallback(&cbName##Multicast[0], cbMax, (CB_CallbackFuncType)cbFunc, cbDispatchFunc, cbUserData); \
    } \
    BOOL cbName##_IsRegistered(cbName##CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc) { \
        return _CB_IsAdded(&cbName##Multicast[0], cbMax, (CB_CallbackFuncType)cbFunc, cbDispatchFunc); \
    } \
    BOOL cbName##_Unregister(cbName##CallbackFuncType cbFunc, CB_DispatchCallbackFuncType cbDispatchFunc) { \
        return _CB_RemoveCallback(&cbName##Multicast[0], cbMax, (CB_CallbackFuncType)cbFunc, cbDispatchFunc); \
    } \
    BOOL cbName##_Invoke(cbArg cbData) { \
        return _CB_Dispatch(&cbName##Multicast[0], cbMax, cbData, cbArgSize); \
    } \
    BOOL cbName##_InvokeArray(cbArg cbData, size_t num, size_t size) { \
        return _CB_Dispatch(&cbName##Multicast[0], cbMax, cbData, num * size); \
    } \
    const CB_Info* cbName##_GetCbInfo(unsigned int cbIdx) { \
        if (cbIdx >= cbMax) return NULL; \
        return &cbName##Multicast[cbIdx]; \
    } 

// Initialization function called one time at startup
void CB_Init(void);

// Terminate function called one time at shutdown
void CB_Term(void);

// Called by a target OS task to invoke the callback function
void CB_TargetInvoke(const CB_CallbackMsg* cbMsg);

// Private functions. Do not call these functions directly.
BOOL _CB_AddCallback(CB_Info* cbInfo, size_t cbInfoLen, CB_CallbackFuncType cbFunc,
    CB_DispatchCallbackFuncType cbDispatchFunc, void* cbUserData);
BOOL _CB_IsAdded(CB_Info* cbInfo, size_t cbInfoLen, CB_CallbackFuncType cbFunc,
    CB_DispatchCallbackFuncType cbDispatchFunc);
BOOL _CB_RemoveCallback(CB_Info* cbInfo, size_t cbInfoLen, CB_CallbackFuncType cbFunc,
    CB_DispatchCallbackFuncType cbDispatchFunc);
BOOL _CB_Dispatch(const CB_Info* cbInfo, size_t cbInfoLen, const void* cbData, size_t cbDataSize);

#ifdef __cplusplus
}
#endif

#endif