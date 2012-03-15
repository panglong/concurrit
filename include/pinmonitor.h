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
#include "predicate.h"
#include "scenario.h"

namespace concurrit {

/// class to handle pin events
class PinMonitor {
private:
	static PinMonitor* instance_;
	PinMonitor(){}

public:
	~PinMonitor(){}

	static PinMonitor* GetInstance();


	MemoryCellBase* GetMemoryCell(void* addr, uint32_t size) {
		switch(size) {
		case 1:
			return new MemoryCell<uint8_t>(static_cast<uint8_t*>(addr));
		case 2:
			return new MemoryCell<uint16_t>(static_cast<uint16_t*>(addr));
		case 4:
			return new MemoryCell<uint32_t>(static_cast<uint32_t*>(addr));
		case 8:
			return new MemoryCell<uint64_t>(static_cast<uint64_t*>(addr));
		default:
			safe_assert(false);
			break;
		}
		return NULL;
	}

	SharedAccess* GetSharedAccess(AccessType type, MemoryCellBase* cell) {
		return new SharedAccess(type, cell);
	}

	Scenario* GetScenario() {
		return NULL;
	}

	/******************************************************************************************/

	// callbacks
	void MemWriteBefore(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL) {
		safe_assert(loc != NULL);
		printf("Writing before %s\n", loc->ToString().c_str());

		Scenario* scenario = GetScenario();
		State* state = scenario->state();

		state->P_BeforeMemWrite << tid << addr = true;

		// TODO(elmas)
		MemoryCellBase* cell = GetMemoryCell(addr, size);
		SharedAccess* access = GetSharedAccess(WRITE_ACCESS, cell);


		state->P_BeforeMemWrite << tid << addr = false;
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
