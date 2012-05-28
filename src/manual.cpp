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

#include "dummy.h"

// functions implementing manual instrumentation routines
// these functions overwrite functions in libdummy.so when concurrit is preloaded

using namespace concurrit;

/********************************************************************************/

void concurritStartInstrumentEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	if(filename != NULL) current->set_srcloc(new SourceLocation(filename, funcname, line));

	StartInstrument(); // notify pintool
}

/********************************************************************************/

void concurritEndInstrumentEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	if(filename != NULL) current->set_srcloc(new SourceLocation(filename, funcname, line));

	EndInstrument(); // notify pintool

	// reset aux variables of current thread
	current->FinishControlledTransition();
}

/********************************************************************************/

void concurritAtPcEx(int pc, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::AtPc(current, scenario, pc, new SourceLocation(filename, funcname, line));
}

/********************************************************************************/

void concurritFuncEnterEx(void* addr, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::FuncEnter(current, scenario, PTR2ADDRINT(addr), new SourceLocation(filename, funcname, line), 0, 0);
}

/********************************************************************************/

void concurritFuncReturnEx(void* addr, const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());

	PinMonitor::FuncReturn(current, scenario, PTR2ADDRINT(addr), new SourceLocation(filename, funcname, line), 0);
}

/********************************************************************************/

void TriggerAssert(const char* expr, const char* filename, const char* funcname, int line) {
	MYLOG(1) << "Triggering assertion violation!";
	TRIGGER_ASSERTION_VIOLATION(expr, filename, funcname, line);
}

/********************************************************************************/

void concurritThreadEndEx(const char* filename, const char* funcname, int line) {
	if(!PinMonitor::IsEnabled() || !Config::ManualInstrEnabled) return;

	Coroutine* current = safe_notnull(Coroutine::Current());
	Scenario* scenario = safe_notnull(safe_notnull(current->group())->scenario());
	if(filename != NULL) current->set_srcloc(new SourceLocation(filename, funcname, line));

	PinMonitor::ThreadEnd(current, scenario);
}

/********************************************************************************/


