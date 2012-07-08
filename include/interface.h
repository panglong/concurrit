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
#include "sharedaccess.h"

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
	AtPc			= 12U,
	// events from DSL to SUT
	Continue		= 13U,
	TestShutdown	= 14U,
	// events exchanged internally
	ThreadEndIntern = 15U,
	AddressOfSymbol = 16U;

/***********************************************************************/

inline const char* EventKindToString(EventKind& kind) {
#define EventKindToStringMacro(k)	case k: return #k;
	switch(kind) {
	EventKindToStringMacro(InvalidEventKind)
	EventKindToStringMacro(MemAccessBefore)
	EventKindToStringMacro(MemAccessAfter)
	EventKindToStringMacro(MemWrite)
	EventKindToStringMacro(MemRead)
	EventKindToStringMacro(FuncCall)
	EventKindToStringMacro(FuncEnter)
	EventKindToStringMacro(FuncReturn)
	EventKindToStringMacro(ThreadStart)
	EventKindToStringMacro(ThreadEnd)
	EventKindToStringMacro(TestStart)
	EventKindToStringMacro(TestEnd)
	EventKindToStringMacro(AtPc)
	EventKindToStringMacro(Continue)
	EventKindToStringMacro(TestShutdown)
	EventKindToStringMacro(ThreadEndIntern)
	EventKindToStringMacro(AddressOfSymbol)
	default:
		safe_fail("Unknown event kind %d\n", kind);
		break;
	}
	return "unknown";
#undef EventKindToStringMacro
}

/***********************************************************************/

struct EventBuffer {
public:
	EventKind type;
	THREADID threadid;
	ADDRINT addr;
	ADDRINT addr_target;
	uint32_t size;
	uint32_t direct;
	ADDRINT arg0;
	ADDRINT arg1;
	ADDRINT retval;
	int32_t pc;
	SourceLocation* loc_src;
	SourceLocation* loc_target;
	char str[64];

	EventBuffer() { Clear(); }

	void Clear() {
		type = (InvalidEventKind);
		threadid = (INVALID_THREADID);
		addr = (0);
		addr_target = (0);
		size = (0);
		direct = (true);
		arg0 = (0);
		arg1 = (0);
		retval = (0);
		pc = (0);
		loc_src = (NULL);
		loc_target = (NULL);
		memset(str, 0, 64 * sizeof(char));
		str[0] = '\0';
	}

	bool Check() {
		return type != InvalidEventKind && threadid != INVALID_THREADID;
	}

	EventBuffer& operator=(const EventBuffer& e) {
		if(this != &e) {
			type = e.type;
			threadid = e.threadid;
			loc_src = e.loc_src;
			switch(e.type) {
			case MemRead:
			case MemWrite:
				addr = e.addr;
				size = e.size;
				break;
			case FuncEnter:
				addr = e.addr;
				arg0 = e.arg0;
				arg1 = e.arg1;
				break;
			case FuncReturn:
				addr = e.addr;
				retval = e.retval;
				break;
			case FuncCall:
				addr = e.addr;
				addr_target = e.addr_target;
				direct = e.direct;
				arg0 = e.arg0;
				arg1 = e.arg1;
				loc_target = e.loc_target;
				break;
			case AtPc:
				pc = e.pc;
				break;
			case AddressOfSymbol:
				addr = e.addr;
				memcpy(str, e.str, 64);
				break;
			default:
				break;
			}
		}
		return *this;
	}
};

/********************************************************************************/

} // end namespace



#endif /* INTERFACE_H_ */
