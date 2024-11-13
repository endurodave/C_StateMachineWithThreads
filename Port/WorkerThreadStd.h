// https://github.com/endurodave/StdWorkerThread
// David Lafreniere

#ifndef _THREAD_STD_H
#define _THREAD_STD_H

#include "callback.h"
#include "DataTypes.h"
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>

// C language interface to callback dispatch functions
extern "C" void CreateThreads(void);
extern "C" void ExitThreads(void);
extern "C" BOOL DispatchCallbackThread1(const CB_CallbackMsg* cbMsg);
extern "C" BOOL DispatchCallbackThread2(const CB_CallbackMsg* cbMsg);

class ThreadMsg;

class WorkerThread 
{
public:
	/// Constructor
	WorkerThread(const CHAR* threadName);

	/// Destructor
	~WorkerThread();

	/// Called once to create the worker thread
	/// @return TRUE if thread is created. FALSE otherise. 
	BOOL CreateThread();

	/// Called once a program exit to exit the worker thread
	void ExitThread();

	/// Get the ID of this thread instance
	std::thread::id GetThreadId();

	/// Get the ID of the currently executing thread
	static std::thread::id GetCurrentThreadId();

	virtual void DispatchCallback(const CB_CallbackMsg* msg);

private:
	WorkerThread(const WorkerThread&);
	WorkerThread& operator=(const WorkerThread&);

	/// Entry point for the thread
	void Process();

    /// Entry point for timer thread
    void TimerThread();

	std::thread* m_thread;
	std::queue<ThreadMsg*> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_cv;
    std::atomic<bool> m_timerExit;
	const std::string THREAD_NAME;
};

#endif 

