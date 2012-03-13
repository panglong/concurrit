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


#ifndef PINMONITOR_H_
#define PINMONITOR_H_

#include "common.h"
#include "sharedaccess.h"

namespace concurrit {

/// class to handle pin events
class PinMonitor {
private:
	static PinMonitor* instance_;
	PinMonitor(){}

public:
	~PinMonitor(){}

	static PinMonitor* GetInstance();


	// callbacks
	void MemWriteBefore(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL) {
		safe_assert(loc != NULL);
		printf("Writing before %s\n", loc->ToString().c_str());
	}

	void MemWriteAfter(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL) {
		safe_assert(loc != NULL);
		printf("Writing after %s\n", loc->ToString().c_str());
	}

	void MemReadBefore(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL) {
		safe_assert(loc != NULL);
		printf("Reading before %s\n", loc->ToString().c_str());
	}

	void MemReadAfter(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL) {
		safe_assert(loc != NULL);
		printf("Reading after %s\n", loc->ToString().c_str());
	}

	void FuncCall(THREADID threadid, void* addr, bool direct, SourceLocation* loc_src, SourceLocation* loc_target) {
		printf("Calling before: %s\n", loc_target->funcname().c_str());
	}

	void FuncEnter(THREADID threadid, void* addr, SourceLocation* loc) {
		printf("Entering: %s\n", loc->funcname().c_str());
	}

	void FuncReturn(THREADID threadid, void* addr, SourceLocation* loc, ADDRINT retval) {
		printf("Returning: %s\n", loc->funcname().c_str());
	}

};

} // end namespace



#endif /* PINMONITOR_H_ */
