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

//Coroutine* PinMonitor::tid_to_coroutine_[MAX_THREADS];
volatile bool PinMonitor::enabled_ = false;
volatile bool PinMonitor::down_ = false;
SymbolToAddressMap PinMonitor::symbol_to_address_;
PinToolOptions PinMonitor::options_ = {
		TRUE, // TrackFuncCalls;
		TRUE,  // InstrTopLevelFuncs
		FALSE,  // InstrAfterMemoryAccess
};

/********************************************************************************/


void PinMonitor::Init() {
	// clean tid_to_coroutine
//	memset(tid_to_coroutine_, NULL, (sizeof(Coroutine*) * MAX_THREADS));
//	for(int i = 0; i < MAX_THREADS; ++i) {
//		tid_to_coroutine_[i] = NULL;
//	}
	if(!down_) {
		// init pin tool options, detected by Pin
		InitPinTool(&PinMonitor::options_);
	}
}

//Coroutine* PinMonitor::GetCoroutineByTid(THREADID tid) {
//	safe_assert(0 <= tid && tid < MAX_THREADS);
//	Coroutine* co = tid_to_coroutine_[tid];
//	if(co == NULL) {
//		co = Coroutine::Current();
//		tid_to_coroutine_[tid] = safe_notnull(co);
//	}
//	return co;
//}
//
//
//MemoryCellBase* PinMonitor::GetMemoryCell(void* addr, uint32_t size) {
//	switch(size) {
//	case 1:
//		return new MemoryCell<uint8_t>(static_cast<uint8_t*>(addr));
//	case 2:
//		return new MemoryCell<uint16_t>(static_cast<uint16_t*>(addr));
//	case 4:
//		return new MemoryCell<uint32_t>(static_cast<uint32_t*>(addr));
//	case 8:
//		return new MemoryCell<uint64_t>(static_cast<uint64_t*>(addr));
//	default:
//		safe_assert(false);
//		break;
//	}
//	return NULL;
//}
//
//SharedAccess* PinMonitor::GetSharedAccess(AccessType type, MemoryCellBase* cell) {
//	return new SharedAccess(type, cell);
//}

/******************************************************************************************/

void PinMonitor::Enable() {
	if(!Config::RunUncontrolled) {
		MYLOG(2) << ">>> Enabling instrumentation.";
		if(Config::PinInstrEnabled && !down_) EnablePinTool();
		enabled_ = true;
	}
}
void PinMonitor::Disable() {
	if(!Config::RunUncontrolled) {
		MYLOG(2) << ">>> Disabling instrumentation.";
		if(Config::PinInstrEnabled && !down_) DisablePinTool();
		enabled_ = false;
	}
}
/******************************************************************************************/

// ! Do not disable pinmonitor, as the enabled_ flag is used other places
void PinMonitor::Shutdown() {
	if(!down_) {
		MYLOG(2) << ">>> Shutting down pintool.";
		ShutdownPinTool();
		down_ = true;
	}
}

/******************************************************************************************/

// callbacks

void PinMonitor::AddressOfSymbol(const char* symbol, ADDRINT addr) {
	safe_assert(symbol != NULL);
	safe_check(strnlen(symbol, 64) < 64);

	symbol_to_address_[std::string(symbol)] = addr;

	MYLOG(1) << "Updated address of symbol " << symbol << " to " << ADDRINT2PTR(addr);
}

ADDRINT PinMonitor::GetAddressOfSymbol(const std::string& symbol, ADDRINT addr /*= ADDRINT(0)*/) {
	SymbolToAddressMap::iterator itr = symbol_to_address_.find(symbol);
	if(itr != symbol_to_address_.end()) {
		return itr->second;
	}
	return addr;
}

/********************************************************************************/

void PinMonitor::MemAccessBefore(Coroutine* current, Scenario* scenario, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
//	safe_assert(loc != NULL);

	current->set_srcloc(loc);
	scenario->OnControlledTransition(current);
}

/********************************************************************************/

void PinMonitor::MemAccessAfter(Coroutine* current, Scenario* scenario, SourceLocation* loc /*= NULL*/) {
//	safe_assert(current != NULL && scenario != NULL);
//	safe_assert(loc != NULL);
//
//	current->set_srcloc(loc);
//	scenario->AfterControlledTransition(current);
}

/********************************************************************************/

void PinMonitor::MemWrite(Coroutine* current, Scenario* scenario, ADDRINT addr, uint32_t size, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
//	safe_assert(loc != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"MemWrite by %d to %lx size %d",
				current->tid(), addr, size);
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	// update auxstate
	AuxState::Writes->set(addr, size, current->tid());

	current->set_srcloc(loc);
}

/********************************************************************************/

void PinMonitor::MemRead(Coroutine* current, Scenario* scenario, ADDRINT addr, uint32_t size, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
//	safe_assert(loc != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"MemRead by %d from %lx size %d",
				current->tid(), addr, size);
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	// update auxstate
	AuxState::Reads->set(addr, size, current->tid());

	current->set_srcloc(loc);
}

/********************************************************************************/

void PinMonitor::FuncCall(Coroutine* current, Scenario* scenario, ADDRINT addr_src, ADDRINT addr_target, bool direct, SourceLocation* loc_src, SourceLocation* loc_target, ADDRINT arg0, ADDRINT arg1) {
	safe_assert(loc_src != NULL && loc_target != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"FuncCall[%s] by %d from %s(%lx) to %s(%lx)",
				(direct ? "direct" : "indirect"), current->tid(), loc_src->funcname().c_str(), addr_src, loc_target->funcname().c_str(), addr_target);
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	// update auxstate
	AuxState::CallsFrom->set(addr_src, current->tid());
	AuxState::CallsTo->set(addr_target, current->tid());

	AuxState::Arg0->set(addr_target, arg0, current->tid());
	AuxState::Arg1->set(addr_target, arg1, current->tid());

	current->set_srcloc(loc_src);
	scenario->OnControlledTransition(current);
}

/********************************************************************************/

void PinMonitor::FuncEnter(Coroutine* current, Scenario* scenario, ADDRINT addr, SourceLocation* loc, ADDRINT arg0, ADDRINT arg1) {
	safe_assert(current != NULL && scenario != NULL);
//	safe_assert(loc != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"FuncEnter by %d to %s(%lx) with arg0 %lu arg1 %lu",
				current->tid(), loc->funcname().c_str(), addr, arg0, arg1);
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	// update auxstate
	AuxState::Enters->set(addr, true, current->tid());

	int c = AuxState::InFunc->get(addr, current->tid());
	safe_assert(c >= 0);
	AuxState::InFunc->set(addr, c+1, current->tid());

	c = AuxState::NumInFunc->get(addr, current->tid());
	safe_assert(c >= 0);
	AuxState::NumInFunc->set(addr, c+1, current->tid());

	AuxState::Arg0->set(addr, arg0, current->tid());
	AuxState::Arg1->set(addr, arg1, current->tid());

	current->set_srcloc(loc);
	scenario->OnControlledTransition(current);
}

/********************************************************************************/

void PinMonitor::FuncReturn(Coroutine* current, Scenario* scenario, ADDRINT addr, SourceLocation* loc, ADDRINT retval) {
	safe_assert(current != NULL && scenario != NULL);
//	safe_assert(loc != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"FuncReturn by %d from %s(%lx) with ret %lu",
				current->tid(), loc->funcname().c_str(), addr, retval);
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	// update auxstate
	AuxState::Returns->set(addr, true, current->tid());

	AuxState::RetVal->set(addr, retval, current->tid());

	current->set_srcloc(loc);
	scenario->OnControlledTransition(current);

	int c = AuxState::InFunc->get(addr, current->tid());
	safe_assert(c >= 0);
	if(c == 0) return; // TODO(elmas): in general c must be > 1. Sometimes (radbench) it is 0!
	AuxState::InFunc->set(addr, c-1, current->tid());

	AuxState::Arg0->set(addr, 0, current->tid());
	AuxState::Arg1->set(addr, 0, current->tid());

//	scenario->AfterControlledTransition(current);
}

/********************************************************************************/

void PinMonitor::ThreadEnd(Coroutine* current, Scenario* scenario) {
	safe_assert(current != NULL && scenario != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"ThreadEnd by %d",
				current->tid());
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	// last controlled transition
	// set predicate for "will end" before this transition
	safe_assert(current->status() == ENABLED);
	THREADID tid = current->tid();

	safe_assert(!AuxState::Ends->isset(tid));
	AuxState::Ends->set(true, tid);
	safe_assert(AuxState::Ends->isset(tid));

	scenario->OnControlledTransition(current);

	safe_assert(AuxState::Ends->isset(tid));
}

/********************************************************************************/

void PinMonitor::AtPc(Coroutine* current, Scenario* scenario, int pc, SourceLocation* loc) {
	safe_assert(current != NULL && scenario != NULL);

	if(Config::SaveExecutionTraceToFile) {
		snprintf(current->instr_callback_info(), 256,
				"AtPc by %d at %d",
				current->tid(), pc);
		MYLOG(2) << "Calling pinmonitor " << current->instr_callback_info();
	}

	AuxState::Pc->set(pc, current->tid());
	AuxState::AtPc->set(true, current->tid());

	current->set_srcloc(loc);
	scenario->OnControlledTransition(current);
}

/********************************************************************************/

void CallPinMonitor(EventBuffer* event) {
	safe_assert(event != NULL);
	safe_assert(!PinMonitor::IsDown());

	if(event->type == AddressOfSymbol) {
		PinMonitor::AddressOfSymbol(event->str, event->addr);
		return;
	}

	if(!PinMonitor::IsEnabled()) {
		return;
	}

//	Coroutine* current = safe_notnull(PinMonitor::GetCoroutineByTid(event->threadid));
	Coroutine* current = safe_notnull(Coroutine::Current());
	safe_assert(!current->IsMain());
	Scenario* scenario = safe_notnull(Scenario::Current());

	switch(event->type) {
		case MemAccessBefore:
			PinMonitor::MemAccessBefore(current, scenario, event->loc_src);
			break;
		case MemAccessAfter:
			PinMonitor::MemAccessAfter(current, scenario, event->loc_src);
			break;
		case MemWrite:
			PinMonitor::MemWrite(current, scenario, event->addr, event->size, event->loc_src);
			break;
		case MemRead:
			PinMonitor::MemRead(current, scenario, event->addr, event->size, event->loc_src);
			break;
		case FuncEnter:
			PinMonitor::FuncEnter(current, scenario, event->addr, event->loc_src, event->arg0, event->arg1);
			break;
		case FuncReturn:
			PinMonitor::FuncReturn(current, scenario, event->addr, event->loc_src, event->retval);
			break;
		case FuncCall:
			PinMonitor::FuncCall(current, scenario, event->addr, event->addr_target, event->direct == TRUE, event->loc_src, event->loc_target, event->arg0, event->arg1);
			break;
		case ThreadEnd:
			PinMonitor::ThreadEnd(current, scenario);
			break;
		case AtPc:
			PinMonitor::AtPc(current, scenario, event->pc, event->loc_src);
			break;
		default:
			safe_fail("Unrecognized event type: %d\n", event->type);
			break;
	}
}

/********************************************************************************/

// functions whose first instructions to be instrumented by pin tool

void InitPinTool(PinToolOptions* options) { MYLOG(2) << "Initializing pin instrumentation"; }

void EnablePinTool() { MYLOG(2) << "Enabling pin instrumentation"; }
void DisablePinTool() { MYLOG(2) << "Disabling pin instrumentation"; }

void ThreadRestart() { MYLOG(2) << "Restarting thread."; }

void ShutdownPinTool() { MYLOG(2) << "Shutting down pintool."; }

void StartInstrument() { MYLOG(2) << "Starting an instrumented function."; }
void EndInstrument() { MYLOG(2) << "Ending an instrumented function."; }

/********************************************************************************/

} // end namespace


