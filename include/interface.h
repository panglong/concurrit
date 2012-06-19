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


#ifndef INTERFACE_H_
#define INTERFACE_H_

#include "common.h"

namespace concurrit {

/***********************************************************************/

struct PinToolOptions {
	uint32_t TrackFuncCalls;
	uint32_t InstrTopLevelFuncs;
	uint32_t InstrAfterMemoryAccess;
};

/***********************************************************************/

typedef uint32_t EventKind;
const EventKind
	InvalidEventKind = 0,
	MemAccessBefore	= 1U,
	MemAccessAfter	= 2U,
	MemWrite 		= 3U,
	MemRead 		= 4U,
	FuncCall 		= 5U,
	FuncEnter 		= 6U,
	FuncReturn 		= 7U,
	ThreadStart		= 8U,
	ThreadEnd		= 9U,
	TestStart		= 10U,
	TestEnd			= 11U,
	// events from DSL to SUT
	Continue		= 12U,
	TestShutdown	= 13U,
	// events exchanged internally
	ThreadEndInternal = 14U;

/***********************************************************************/

struct EventBuffer {
	EventKind type;
	THREADID threadid;
	ADDRINT addr;
	ADDRINT addr_target;
	uint32_t size;
	uint32_t direct;
	ADDRINT arg0;
	ADDRINT arg1;
	ADDRINT retval;
	SourceLocation* loc_src;
	SourceLocation* loc_target;

	EventBuffer() :
		type(InvalidEventKind),
		threadid(INVALID_THREADID),
		addr(0),
		addr_target(0),
		size(0),
		direct(true),
		arg0(0),
		arg1(0),
		retval(0),
		loc_src(NULL),
		loc_target(NULL) {}
};

/********************************************************************************/

} // end namespace



#endif /* INTERFACE_H_ */
