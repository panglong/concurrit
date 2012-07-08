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

#include "instrument.h"

/********************************************************************************/

#define dummy_error()	fprintf(stderr, "Function %s in dummy.cpp should not be called!!!\n", __FUNCTION__); \
						fflush(stderr); \
						exit(EXIT_FAILURE);

/********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void concurritInit() {dummy_error();}

void concurritAddressOfSymbolEx(const char* symbol, uintptr_t addr) {dummy_error();}

void concurritStartTest() {dummy_error();}
void concurritEndTest() {dummy_error();}

void concurritEndSearch() {dummy_error();}

void concurritStartInstrumentEx(const char* filename, const char* funcname, int line) {dummy_error();}
void concurritEndInstrumentEx(const char* filename, const char* funcname, int line) {dummy_error();}

void concurritAtPcEx(int pc, const char* filename, const char* funcname, int line) {dummy_error();}

void concurritFuncEnterEx(void* addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {dummy_error();}
void concurritFuncReturnEx(void* addr, uintptr_t retval, const char* filename, const char* funcname, int line) {dummy_error();}

void concurritFuncCallEx(void* from_addr, void* to_addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {dummy_error();}

void concurritMemReadEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {dummy_error();}
void concurritMemWriteEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {dummy_error();}

void concurritMemAccessBeforeEx(const char* filename, const char* funcname, int line) {dummy_error();}
void concurritMemAccessAfterEx(const char* filename, const char* funcname, int line) {dummy_error();}

void concurritThreadStartEx(const char* filename, const char* funcname, int line)  {dummy_error();}
void concurritThreadEndEx(const char* filename, const char* funcname, int line)  {dummy_error();}

void concurritTriggerAssert(const char* expr, const char* filename, const char* funcname, int line) {dummy_error();}

#ifdef __cplusplus
} // extern "C"
#endif

/********************************************************************************/

