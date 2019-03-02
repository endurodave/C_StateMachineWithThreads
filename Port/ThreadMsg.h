#ifndef _THREAD_MSG_H
#define _THREAD_MSG_H

#include "DataTypes.h"

/// @brief A class to hold a platform-specific thread messsage that will be passed 
/// through the OS message queue. 
class ThreadMsg
{
public:
	/// Constructor
	/// @param[in] id - a unique identifier for the thread messsage
	/// @param[in] data - a pointer to the messsage data to be typecast
	///		by the receiving task based on the id value. 
	/// @pre The data pointer argument *must* be created on the heap or fixed 
    ///     block allocator.
	/// @port The destination thread will delete the heap allocated data once the 
	///		callback is complete.  
	ThreadMsg(INT id, const void* data) : 
		m_id(id), 
		m_data(data)
	{
	}

	INT GetId() const { return m_id; } 
	const void* GetData() const { return m_data; } 

private:
	INT m_id;
	const void* m_data;
};

#endif // _THREAD_MSG_H
