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

/********************************************************************************/

void ConcurritInstrHandler::concurritStartTest() {
	if(Config::RunUncontrolled) return;

	MYLOG(1) << "concurritStartTest";

	PinMonitor::Enable();

	// signal test_start condition
	Concurrit::sem_test_start()->Signal();
}

void ConcurritInstrHandler::concurritEndTest() {
	if(Config::RunUncontrolled) return;

	MYLOG(1) << "concurritEndTest";

	PinMonitor::Disable();

	// throw backtrack if there is a node waiting to be consumed
	Scenario::NotNullCurrent()->exec_tree()->EndWithBacktrack(Coroutine::Current(), THREADS_ALLENDED, "concurritEndTest");
}

void ConcurritInstrHandler::concurritEndSearch() {
	MYLOG(1) << "concurritEndSearch";

	TRIGGER_TERMINATE_SEARCH();
}

/********************************************************************************/

void ConcurritInstrHandler::concurritAddressOfSymbolEx(const char* symbol, uintptr_t addr) {
	if(Config::RunUncontrolled) return;

	PinMonitor::AddressOfSymbol(symbol, addr);
}

/********************************************************************************/

void ConcurritInstrHandler::concurritStartInstrumentEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || Config::RunUncontrolled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	if(filename != NULL) current->set_srcloc(new SourceLocation(filename, funcname, line));

	StartInstrument(); // notify pintool
}

/********************************************************************************/

void ConcurritInstrHandler::concurritEndInstrumentEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || Config::RunUncontrolled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	if(filename != NULL) current->set_srcloc(new SourceLocation(filename, funcname, line));

	EndInstrument(); // notify pintool

	// reset aux variables of current thread
	current->FinishControlledTransition();
}

/********************************************************************************/

void ConcurritInstrHandler::concurritAtPcEx(int pc, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled()) return;
//	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::AtPc(current, scenario, pc, new SourceLocation(filename, funcname, line));
}

/********************************************************************************/

void ConcurritInstrHandler::concurritFuncEnterEx(void* addr, ADDRINT arg0, ADDRINT arg1, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::FuncEnter(current, scenario, PTR2ADDRINT(addr), new SourceLocation(filename, funcname, line), arg0, arg1);
}

/********************************************************************************/

void ConcurritInstrHandler::concurritFuncReturnEx(void* addr, ADDRINT retval, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::FuncReturn(current, scenario, PTR2ADDRINT(addr), new SourceLocation(filename, funcname, line), retval);
}

/********************************************************************************/

void ConcurritInstrHandler::concurritFuncCallEx(void* from_addr, void* to_addr, ADDRINT arg0, ADDRINT arg1, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::FuncCall(current, scenario, PTR2ADDRINT(from_addr), PTR2ADDRINT(to_addr), true, new SourceLocation(filename, funcname, line), NULL, arg0, arg1);
}

/********************************************************************************/

void ConcurritInstrHandler::concurritMemReadEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::MemRead(current, scenario, PTR2ADDRINT(addr), (uint32_t)size, new SourceLocation(filename, funcname, line));
}

/********************************************************************************/

void ConcurritInstrHandler::concurritMemWriteEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::MemWrite(current, scenario, PTR2ADDRINT(addr), (uint32_t)size, new SourceLocation(filename, funcname, line));
}

/********************************************************************************/

void ConcurritInstrHandler::concurritMemAccessBeforeEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::MemAccessBefore(current, scenario, new SourceLocation(filename, funcname, line));
}

/********************************************************************************/

void ConcurritInstrHandler::concurritMemAccessAfterEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::MemAccessAfter(current, scenario, new SourceLocation(filename, funcname, line));
}

/********************************************************************************/

void ConcurritInstrHandler::concurritTriggerAssert(const char* expr, const char* filename, const char* funcname, int line) {
	MYLOG(1) << "Triggering assertion violation!";
	TRIGGER_ASSERTION_VIOLATION(expr, filename, funcname, line);
}

/********************************************************************************/

void ConcurritInstrHandler::concurritThreadStartEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;
	// noop
}

void ConcurritInstrHandler::concurritThreadEndEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());
	if(filename != NULL) current->set_srcloc(new SourceLocation(filename, funcname, line));

	PinMonitor::ThreadEnd(current, scenario);
}

/********************************************************************************/

} // end namespace

