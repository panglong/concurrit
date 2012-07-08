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

#ifndef INSTRUMENT_H_
#define INSTRUMENT_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>

/********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern void concurritInit();

extern void concurritAddressOfSymbolEx(const char* symbol, uintptr_t addr);

extern void concurritStartTest();
extern void concurritEndTest();

extern void concurritEndSearch();

extern void concurritStartInstrumentEx(const char* filename, const char* funcname, int line);
extern void concurritEndInstrumentEx(const char* filename, const char* funcname, int line);

extern void concurritAtPcEx(int pc, const char* filename, const char* funcname, int line);

extern void concurritFuncEnterEx(void* addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line);
extern void concurritFuncReturnEx(void* addr, uintptr_t retval, const char* filename, const char* funcname, int line);

extern void concurritFuncCallEx(void* from_addr, void* to_addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line);

extern void concurritMemReadEx(void* addr, size_t size, const char* filename, const char* funcname, int line);
extern void concurritMemWriteEx(void* addr, size_t size, const char* filename, const char* funcname, int line);

extern void concurritMemAccessBeforeEx(const char* filename, const char* funcname, int line);
extern void concurritMemAccessAfterEx(const char* filename, const char* funcname, int line);

extern void concurritThreadStartEx(const char* filename, const char* funcname, int line);
extern void concurritThreadEndEx(const char* filename, const char* funcname, int line);

extern void concurritTriggerAssert(const char* expr, const char* filename, const char* funcname, int line);

#ifdef __cplusplus
} // extern "C"
#endif

/********************************************************************************/

#define concurritAddressOfSymbol(s, a)	concurritAddressOfSymbolEx((s), (uintptr_t)(a))

#define concurritStartInstrument()		concurritStartInstrumentEx(__FILE__, __PRETTY_FUNCTION__, __LINE__)
#define concurritEndInstrument()		concurritEndInstrumentEx(__FILE__, __PRETTY_FUNCTION__, __LINE__)

#define concurritAtPc(c)				concurritAtPcEx((c), __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define concurritFuncEnter(a, b, c)		concurritFuncEnterEx((void*)(a), (uintptr_t)(b), (uintptr_t)(c), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define concurritFuncReturn(a, b)		concurritFuncReturnEx((void*)(a), (uintptr_t)(b), __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define concurritFuncCall(a, b, c, d)	concurritFuncCallEx((void*)(a), (void*)(b), (uintptr_t)(c), (uintptr_t)(d), __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define concurritMemRead(a, s)			concurritMemReadEx((void*)(a), (s), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define concurritMemWrite(a, s)			concurritMemWriteEx((void*)(a), (s), __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define concurritMemAccessBefore()		concurritMemAccessBeforeEx(__FILE__, __PRETTY_FUNCTION__, __LINE__)
#define concurritMemAccessAfter()		concurritMemAccessAfterEx(__FILE__, __PRETTY_FUNCTION__, __LINE__)

#define concurritThreadStart()			concurritThreadStartEx(__FILE__, __PRETTY_FUNCTION__, __LINE__)
#define concurritThreadEnd()			concurritThreadEndEx(__FILE__, __PRETTY_FUNCTION__, __LINE__)

/********************************************************************************/

#define concurritControl()			concurritAtPc(0xABCDEF)

/********************************************************************************/

#define concurritAssert(b)			if(!(b)) { fprintf(stderr, "ASSERTION VIOLATION!\n"); concurritTriggerAssert(#b, __FILE__, __PRETTY_FUNCTION__, __LINE__); }

/********************************************************************************/

#ifdef __cplusplus
#define CONCURRIT_TEST_MAIN(main_func)	\
	extern "C" int __main__(int argc, char* argv[]) { \
		concurritStartTest(); 				\
		int ret = main_func(argc, argv);	\
		return ret;							\
	}
#else
#define CONCURRIT_TEST_MAIN(main_func)	\
	int __main__(int argc, char* argv[]) {	\
		concurritStartTest(); 				\
		int ret = main_func(argc, argv);	\
		return ret;							\
	}
#endif

/********************************************************************************/

#endif /* INSTRUMENT_H_ */
