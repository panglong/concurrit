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


// functions implementing manual instrumentation routines
// these functions overwrite functions in libdummy.so when concurrit is preloaded

using namespace concurrit;

/********************************************************************************/

void concurritAddressOfSymbolEx(const char* symbol, uintptr_t addr) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritAddressOfSymbolEx(symbol, addr);
}

void concurritStartTest() {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritStartTest();
}
void concurritEndTest() {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritEndTest();
}

void concurritEndSearch() {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritEndSearch();
}

void concurritStartInstrumentEx(const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritStartInstrumentEx(filename, funcname, line);
}
void concurritEndInstrumentEx(const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritEndInstrumentEx(filename, funcname, line);
}

void concurritAtPcEx(int pc, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritAtPcEx(pc, filename, funcname, line);
}

void concurritFuncEnterEx(void* addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritFuncEnterEx(addr, arg0, arg1, filename, funcname, line);
}
void concurritFuncReturnEx(void* addr, uintptr_t retval, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritFuncReturnEx(addr, retval, filename, funcname, line);
}

void concurritFuncCallEx(void* from_addr, void* to_addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritFuncCallEx(from_addr, to_addr, arg0, arg1, filename, funcname, line);
}

void concurritMemReadEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritMemReadEx(addr, size, filename, funcname, line);
}
void concurritMemWriteEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritMemWriteEx(addr, size, filename, funcname, line);
}

void concurritMemAccessBeforeEx(const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritMemAccessBeforeEx(filename, funcname, line);
}
void concurritMemAccessAfterEx(const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritMemAccessAfterEx(filename, funcname, line);
}

void concurritThreadStartEx(const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritThreadStartEx(filename, funcname, line);
}

void concurritThreadEndEx(const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritThreadEndEx(filename, funcname, line);
}

void concurritTriggerAssert(const char* expr, const char* filename, const char* funcname, int line) {
	safe_assert(InstrHandler::Current != NULL);
	InstrHandler::Current->concurritTriggerAssert(expr, filename, funcname, line);
}

/********************************************************************************/

namespace concurrit {

static InstrHandler DefaultInstHandler;
InstrHandler* InstrHandler::Current = &DefaultInstHandler;

} // end namespace

/********************************************************************************/

