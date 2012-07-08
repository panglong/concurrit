/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "common.h"

namespace concurrit {

#define CHANNEL_BEGIN_ATOMIC()		ScopeMutex m(Channel<MessageType>::mutex());
//									pthread_cleanup_push(Mutex::CleanupHandlerToUnlock, (Channel<MessageType>::mutex())); \

#define CHANNEL_END_ATOMIC()		// Channel<MessageType>::mutex()->Unlock() //pthread_cleanup_pop(0);

#define CHANNEL_T_BEGIN_ATOMIC()	ScopeMutex m(&mutex_);
//									pthread_cleanup_push(Mutex::CleanupHandlerToUnlock, (&mutex_)); \

#define CHANNEL_T_END_ATOMIC()		//pthread_cleanup_pop(0);

// channel for sending and receiving primitive types (T must be primitive)
template <class T>
class Channel {
public:
	Channel() {
		value_ = NULL;
	}
	~Channel() {}

	static void BeginAtomic() {
		mutex_.Lock();
	}

	static void EndAtomic() {
		mutex_.Unlock();
	}

	T& WaitReceive() {
		// acquire lock
		CHANNEL_T_BEGIN_ATOMIC();

		// receive
		this->cv_.Wait(&mutex_);
		this->take();

		// unlock me
		CHANNEL_T_END_ATOMIC();

		return this->buffer_;
	}

	// send value to this channel
	void SendNoWait(const T& value) {
		// acquire lock
		CHANNEL_T_BEGIN_ATOMIC();

		// send
		this->put(value);
		this->cv_.Signal();

		// unlock me
		CHANNEL_T_END_ATOMIC();
	}

	void SendNoWait(const T* value) {
		// acquire lock
		CHANNEL_T_BEGIN_ATOMIC();

		// send
		this->put(value);
		this->cv_.Signal();

		// unlock me
		CHANNEL_T_END_ATOMIC();
	}

	// send value to target and wait to receive from any other source
	T& SendWaitReceive(Channel<T>* target, const T& value) {
		safe_assert(target != NULL);

		// acquire lock
		CHANNEL_T_BEGIN_ATOMIC();

		// send
		target->put(value);
		target->cv_.Signal();

		// receive
		this->cv_.Wait(&mutex_);
		this->take();

		// unlock me
		CHANNEL_T_END_ATOMIC();

		return this->buffer_;
	}

	bool IsEmpty() {
		bool is_empty = (value_ == NULL);
		if(!is_empty) {
			safe_assert(value_ == &buffer_);
		}
		return is_empty;
	}

	void put(const T& value) {
		safe_assert(IsEmpty());
		buffer_ = value;
		value_ = &buffer_;
	}

	void put(const T* value) {
		safe_assert(IsEmpty());
		buffer_ = *value;
		value_ = &buffer_;
	}

	T& take() {
		safe_assert(!IsEmpty());
		value_ = NULL;
		return buffer_;
	}

private:
	DECL_STATIC_FIELD_REF(Mutex, mutex)
	DECL_FIELD_REF(T*, value)
	DECL_FIELD_REF(T, buffer)
	DECL_FIELD_REF(ConditionVar, cv)
};

template<class T>
Mutex Channel<T>::mutex_;


} // end namespace

#endif /* CHANNEL_H_ */
