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

PinMonitor* PinMonitor::instance_ = NULL;

/********************************************************************************/


PinMonitor* PinMonitor::GetInstance() {
	if(instance_ == NULL) {
		instance_ = new PinMonitor();
	}
	return instance_;
}

PinMonitor::PinMonitor() {
	// clean tid_to_coroutine
	for(int i = 0; i < MAX_THREADS; ++i) {
		tid_to_coroutine_[i] = NULL;
	}

	Disable();
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
	printf(">>> Enabling Pin instrumentation\n");
	enabled_ = true;
	ConcurritPinEnable();
}
void PinMonitor::Disable() {
	printf(">>> Disabling Pin instrumentation\n");
	enabled_ = false;
	ConcurritPinDisable();
}

/******************************************************************************************/

// callbacks
void PinMonitor::MemWriteBefore(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc) {
	safe_assert(loc != NULL);
//	printf("Writing before %s\n", loc->funcname().c_str());



//	MemoryCellBase* cell = GetMemoryCell(addr, size);
//	SharedAccess* access = GetSharedAccess(WRITE_ACCESS, cell);

//	current->trinfolist()->push_back(TransitionInfo(MEM_WRITE, access));

	scenario->BeforeControlledTransition(current);
}

void PinMonitor::MemWriteAfter(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc) {
	safe_assert(loc != NULL);
//	printf("Writing after %s\n", loc->funcname().c_str());



//	MemoryCellBase* cell = GetMemoryCell(addr, size);
//	SharedAccess* access = GetSharedAccess(WRITE_ACCESS, cell);

//	current->trinfolist()->push_back(TransitionInfo(MEM_WRITE, access));

	scenario->AfterControlledTransition(current);
}

void PinMonitor::MemReadBefore(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc) {
	safe_assert(loc != NULL);
//	printf("Reading before %s\n", loc->funcname().c_str());


//
//	MemoryCellBase* cell = GetMemoryCell(addr, size);
//	SharedAccess* access = GetSharedAccess(READ_ACCESS, cell);
//
//	current->trinfolist()->push_back(TransitionInfo(MEM_READ, access));
//
	scenario->BeforeControlledTransition(current);
}

void PinMonitor::MemReadAfter(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc) {
	safe_assert(loc != NULL);
//	printf("Reading after %s\n", loc->funcname().c_str());



//	MemoryCellBase* cell = GetMemoryCell(addr, size);
//	SharedAccess* access = GetSharedAccess(WRITE_ACCESS, cell);

//	current->trinfolist()->push_back(TransitionInfo(MEM_WRITE, access));

	scenario->AfterControlledTransition(current);
}

void PinMonitor::FuncCall(Coroutine* current, Scenario* scenario, void* addr, bool direct, SourceLocation* loc_src, SourceLocation* loc_target) {
//	printf("Calling before: %s\n", loc_target->funcname().c_str());



	scenario->OnControlledTransition(current);
}

void PinMonitor::FuncEnter(Coroutine* current, Scenario* scenario, void* addr, SourceLocation* loc) {
//	printf("Entering: %s\n", loc->funcname().c_str());



	scenario->OnControlledTransition(current);
}

void PinMonitor::FuncReturn(Coroutine* current, Scenario* scenario, void* addr, SourceLocation* loc, ADDRINT retval) {
//	printf("Returning: %s\n", loc->funcname().c_str());



	scenario->OnControlledTransition(current);
}


/********************************************************************************/

void CallPinMonitor(PinMonitorCallInfo* info) {
	// check if concurrit is initialized
	if(!IsInitialized) {
		return;
	}

	static PinMonitor* monitor = PinMonitor::GetInstance();
	safe_assert(monitor != NULL);
	safe_assert(info != NULL);

	if(!monitor->enabled()) {
		return;
	}

	Coroutine* current = CHECK_NOTNULL(monitor->GetCoroutineByTid(info->threadid));
	CoroutineGroup* group = current->group();
	// TODO(elmas): when might this happen? (for main?)
	if(group == NULL) {
		return;
	}

	// be sure to call this before the check group == NULL,
	// because IsMain asserts that current->group_ is not null
	if(current->IsMain()) {
		return;
	}

	Scenario* scenario = CHECK_NOTNULL(group->scenario());
	if(scenario->test_status() != TEST_CONTROLLED) {
		return;
	}

	VLOG(2) << "Calling pinmonitor method " << info->type << " threadid " << info->threadid << " addr " << info->addr;

	switch(info->type) {
		case MemWriteBefore:
//			monitor->MemWriteBefore(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case MemWriteAfter:
//			monitor->MemWriteAfter(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case MemReadBefore:
//			monitor->MemReadBefore(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case MemReadAfter:
//			monitor->MemReadAfter(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case FuncCall:
			monitor->FuncCall(current, scenario, info->addr, info->direct, info->loc_src, info->loc_target);
			break;
		case FuncEnter:
			monitor->FuncEnter(current, scenario, info->addr, info->loc_src);
			break;
		case FuncReturn:
			monitor->FuncReturn(current, scenario, info->addr, info->loc_src, info->retval);
			break;
		default:
			printf("Call type: %d\n", info->type);
			bool UnrecognizedPinMonitorCallType = false;
			safe_assert(UnrecognizedPinMonitorCallType);
			break;
	}
}

/********************************************************************************/

void ConcurritPinEnable() { VLOG(2) << "Enabling pin instrumentation"; }
void ConcurritPinDisable() { VLOG(2) << "Disabling pin instrumentation"; }

/********************************************************************************/


} // end namespace


