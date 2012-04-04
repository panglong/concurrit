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


#include "concurrit.h"

namespace concurrit {

Coroutine* PinMonitor::tid_to_coroutine_[MAX_THREADS];
volatile bool PinMonitor::enabled_ = false;

/********************************************************************************/


void PinMonitor::Init() {
	// clean tid_to_coroutine
	for(int i = 0; i < MAX_THREADS; ++i) {
		tid_to_coroutine_[i] = NULL;
	}
	if(!Config::RunUncontrolled) {
		Enable();
	}
}

Coroutine* PinMonitor::GetCoroutineByTid(THREADID tid) {
	safe_assert(0 <= tid && tid < MAX_THREADS);
	Coroutine* co = tid_to_coroutine_[tid];
	if(co == NULL) {
		co = Coroutine::Current();
		tid_to_coroutine_[tid] = co;
	}
	safe_assert(co != NULL);
	return co;
}


MemoryCellBase* PinMonitor::GetMemoryCell(void* addr, uint32_t size) {
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

SharedAccess* PinMonitor::GetSharedAccess(AccessType type, MemoryCellBase* cell) {
	return new SharedAccess(type, cell);
}

/******************************************************************************************/

void PinMonitor::Enable() {
	if(!Config::RunUncontrolled) {
		if(Config::CanEnableDisablePinTool) {
			printf(">>> Enabling Pin instrumentation\n");
			EnablePinTool();
		}
		enabled_ = true;
	}
}
void PinMonitor::Disable() {
	if(!Config::RunUncontrolled) {
		if(Config::CanEnableDisablePinTool) {
			printf(">>> Disabling Pin instrumentation\n");
			DisablePinTool();
		}
		enabled_ = false;
	}
}

/******************************************************************************************/

// callbacks

inline void PinMonitor::MemAccessBefore(Coroutine* current, Scenario* scenario, SourceLocation* loc /*= NULL*/) {
	safe_assert(loc != NULL);

	scenario->BeforeControlledTransition(current);
}

inline void PinMonitor::MemAccessAfter(Coroutine* current, Scenario* scenario, SourceLocation* loc /*= NULL*/) {
	safe_assert(loc != NULL);

	scenario->AfterControlledTransition(current);
}

/********************************************************************************/

inline void PinMonitor::MemWrite(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc /*= NULL*/) {
	safe_assert(loc != NULL);

//	current->trinfolist()->push_back(MemAccessTransitionInfo(TRANS_MEM_WRITE, addr, size, loc));

	// update auxstate
	scenario->aux_state()->Writes.set(reinterpret_cast<ADDRINT>(addr), current->coid());
}

inline void PinMonitor::MemRead(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc /*= NULL*/) {
	safe_assert(loc != NULL);

//	current->trinfolist()->push_back(MemAccessTransitionInfo(TRANS_MEM_READ, addr, size, loc));

	// update auxstate
	scenario->aux_state()->Reads.set(reinterpret_cast<ADDRINT>(addr), current->coid());
}

/********************************************************************************/

inline void PinMonitor::FuncCall(Coroutine* current, Scenario* scenario, void* addr_src, void* addr_target, bool direct, SourceLocation* loc_src, SourceLocation* loc_target) {
	safe_assert(loc_src != NULL && loc_target != NULL);

//	current->trinfolist()->push_back(FuncCallTransitionInfo(addr_src, loc_src, loc_target, direct));

	// update auxstate
	scenario->aux_state()->CallsFrom.set(reinterpret_cast<ADDRINT>(addr_src), current->coid());
	scenario->aux_state()->CallsTo.set(reinterpret_cast<ADDRINT>(addr_target), current->coid());

	scenario->OnControlledTransition(current);
}

inline void PinMonitor::FuncEnter(Coroutine* current, Scenario* scenario, void* addr, SourceLocation* loc) {
	safe_assert(loc != NULL);

//	current->trinfolist()->push_back(FuncEnterTransitionInfo(addr, loc));

	// update auxstate
	scenario->aux_state()->Enters.set(reinterpret_cast<ADDRINT>(addr), current->coid());

	scenario->OnControlledTransition(current);
}

inline void PinMonitor::FuncReturn(Coroutine* current, Scenario* scenario, void* addr, SourceLocation* loc, ADDRINT retval) {
	safe_assert(loc != NULL);

//	current->trinfolist()->push_back(FuncReturnTransitionInfo(addr, loc, retval));

	// update auxstate
	scenario->aux_state()->Returns.set(reinterpret_cast<ADDRINT>(addr), current->coid());

	scenario->OnControlledTransition(current);
}

/********************************************************************************/

void CallPinMonitor(PinMonitorCallInfo* info) {
	safe_assert(info != NULL);

	if(!PinMonitor::IsEnabled()) {
		return;
	}

	Coroutine* current = CHECK_NOTNULL(PinMonitor::GetCoroutineByTid(info->threadid));
	if(current->IsMain()) {
		return;
	}

	VLOG(2) << "Calling pinmonitor method " << info->type << " threadid " << info->threadid << " addr " << info->addr;

	CoroutineGroup* group = CHECK_NOTNULL(current->group());
	Scenario* scenario = CHECK_NOTNULL(group->scenario());

	switch(info->type) {
		case MemAccessBefore:
			PinMonitor::MemAccessBefore(current, scenario, info->loc_src);
			break;
		case MemAccessAfter:
			PinMonitor::MemAccessAfter(current, scenario, info->loc_src);
			break;
		case MemWrite:
			PinMonitor::MemWrite(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case MemRead:
			PinMonitor::MemRead(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case FuncCall:
			PinMonitor::FuncCall(current, scenario, info->addr, info->addr_target, info->direct, info->loc_src, info->loc_target);
			break;
		case FuncEnter:
			PinMonitor::FuncEnter(current, scenario, info->addr, info->loc_src);
			break;
		case FuncReturn:
			PinMonitor::FuncReturn(current, scenario, info->addr, info->loc_src, info->retval);
			break;
		default:
			printf("Call type: %d\n", info->type);
			bool UnrecognizedPinMonitorCallType = false;
			safe_assert(UnrecognizedPinMonitorCallType);
			break;
	}
}

/********************************************************************************/

void EnablePinTool() { VLOG(2) << "Enabling pin instrumentation"; }
void DisablePinTool() { VLOG(2) << "Disabling pin instrumentation"; }

/********************************************************************************/


} // end namespace


