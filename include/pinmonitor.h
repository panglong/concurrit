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
#include "interface.h"
#include "sharedaccess.h"

#define MAX_THREADS 2048

namespace concurrit {

class Coroutine;
class Scenario;

typedef std::map<std::string,ADDRINT> SymbolToAddressMap;

/// class to handle pin events
class PinMonitor {
private:
	PinMonitor();
	~PinMonitor(){}

public:

//	static Coroutine* GetCoroutineByTid(THREADID tid);
//	static MemoryCellBase* GetMemoryCell(void* addr, uint32_t size);
//	static SharedAccess* GetSharedAccess(AccessType type, MemoryCellBase* cell);

	static void Init();

	static void Shutdown();
	static void Enable();
	static void Disable();

	static inline bool IsEnabled() { safe_assert(!enabled_ || Concurrit::IsInitialized()); return enabled_; }
	static inline bool IsDown() { return down_; }

	/******************************************************************************************/

	// callbacks
	static void AddressOfSymbol(const char* symbol, ADDRINT addr);

	static void MemAccessBefore(Coroutine* current, Scenario* scenario, SourceLocation* loc = NULL);
	static void MemAccessAfter(Coroutine* current, Scenario* scenario, SourceLocation* loc = NULL);

	static void MemWrite(Coroutine* current, Scenario* scenario, ADDRINT addr, uint32_t size, SourceLocation* loc = NULL);
	static void MemRead(Coroutine* current, Scenario* scenario, ADDRINT addr, uint32_t size, SourceLocation* loc = NULL);

	static void FuncEnter(Coroutine* current, Scenario* scenario, ADDRINT addr, SourceLocation* loc, ADDRINT arg0, ADDRINT arg1);
	static void FuncReturn(Coroutine* current, Scenario* scenario, ADDRINT addr, SourceLocation* loc, ADDRINT retval);

	static void FuncCall(Coroutine* current, Scenario* scenario, ADDRINT addr_src, ADDRINT addr_target, bool direct, SourceLocation* loc_src, SourceLocation* loc_target, ADDRINT arg0, ADDRINT arg1);

	static void ThreadEnd(Coroutine* current, Scenario* scenario);

	static void AtPc(Coroutine* current, Scenario* scenario, int pc, SourceLocation* loc = NULL);

	/******************************************************************************************/

	static ADDRINT GetAddressOfSymbol(const std::string& symbol, ADDRINT addr = ADDRINT(0));

	/******************************************************************************************/

private:
//	static Coroutine* tid_to_coroutine_[MAX_THREADS];
	static volatile bool enabled_;
	static volatile bool down_;
	static PinToolOptions options_;
	static SymbolToAddressMap symbol_to_address_;
};

/********************************************************************************/

// functions that are called by pin tool

extern "C" void CallPinMonitor(EventBuffer* call_info);

/********************************************************************************/

// functions that are tracked by pin tool

extern "C" void InitPinTool(PinToolOptions* options);

extern "C" void EnablePinTool();
extern "C" void DisablePinTool();

extern "C" void ThreadRestart();

extern "C" void ShutdownPinTool();

extern "C" void StartInstrument();
extern "C" void EndInstrument();

/********************************************************************************/

} // end namespace



#endif /* PINMONITOR_H_ */
