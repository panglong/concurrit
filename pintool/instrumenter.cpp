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

#include "pin.H"
#include "instlib.H"
#include "portability.H"
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <sys/unistd.h>
#include <stdint.h>
#include <sys/time.h>

using namespace INSTLIB;

#include "common.h"
#include "util.h"
#include "statistics.h"
#include "sharedaccess.h"
#include "interface.h"
#include "cmap.h"

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobLogFile(KNOB_MODE_WRITEONCE, "pintool", "log_file",
		"/home/elmas/concurrit/work/pinlogfile.txt", "Specify log file path");

KNOB<string> KnobFinFile(KNOB_MODE_WRITEONCE, "pintool", "finfile",
		"/home/elmas/concurrit/work/finfile.txt", "Specify names of functions to instrument");

KNOB<string> KnobFilteredImagesFile(KNOB_MODE_WRITEONCE, "pintool", "filtered_images_file",
		"/home/elmas/concurrit/work/filtered_images.txt", "Specify names of images to filter out");

KNOB<BOOL> KnobInstrumentTopLevel(KNOB_MODE_WRITEONCE, "pintool",
    "inst_top_level", "0", "Instrument only top-level functions.");

KNOB<BOOL> KnobTrackFuncCalls(KNOB_MODE_WRITEONCE, "pintool",
    "track_func_calls", "0", "Track function calls.");

/* ===================================================================== */

INT32 Usage() {
	cerr << "This pin tool does instrumentation for concurrit\n\n";

	cerr << KNOB_BASE::StringKnobSummary();

	cerr << endl;

	return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

LOCALVAR FILTER filter;
LOCALVAR concurrit::Timer timer("Pin running time");

LOCALVAR ofstream log_file;

//LOCALVAR CONTROL control;
LOCALVAR SKIPPER skipper;
LOCALVAR TLS_KEY tls_key;

//LOCALVAR PIN_LOCK lock;
//#define GLB_LOCK_INIT()		InitLock(&lock)
//#define GLB_LOCK()			GetLock(&lock, 1)
//#define GLB_UNLOCK()			ReleaseLock(&lock)

std::string CONCURRIT_HOME;

LOCALVAR concurrit::PinToolOptions OPTIONS = {
		FALSE, // TrackFuncCalls;
		FALSE, // InstrTopLevelFuncs
		FALSE  // InstrAfterMemoryAccess;
};

//LOCALVAR BOOL INST_TOP_LEVEL = FALSE;
//LOCALVAR BOOL TRACK_FUNC_CALLS = FALSE;

/* ===================================================================== */

#define INSTR_ALL_TRACES	0

/* ===================================================================== */

static const char* NativePinInitFunName = "InitPinTool";
static const char* NativePinShutdownFunName = "ShutdownPinTool";
static const char* NativePinEnableFunName = "EnablePinTool";
static const char* NativePinDisableFunName = "DisablePinTool";
static const char* NativeThreadRestartFunName = "ThreadRestart";
static const char* NativeStartInstrumentFunName = "StartInstrument";
static const char* NativeEndInstrumentFunName = "EndInstrument";

VOID PIN_FAST_ANALYSIS_CALL
PinEnableDisable(UINT32 command);

/* ===================================================================== */

typedef concurrit::ConcurrentMap<ADDRINT,BOOL> PLTMapType;
static PLTMapType rtn_plt_addr;

LOCALFUN INLINE
bool IsPLT(ADDRINT addr) {
	return rtn_plt_addr.contains(addr);
}

LOCALFUN INLINE
void AddToPLT(ADDRINT addr) {
	rtn_plt_addr.insert(addr, TRUE);
}

/* ===================================================================== */

typedef std::vector<ADDRINT> CallStackType;

//class ThreadLocalState {
//public:
//	ThreadLocalState(THREADID tid, bool inst_enabled = false) : tid_(tid), inst_enabled_(inst_enabled) {
//		log_file << "Creating thread local state for tid " << tid_ << endl;
//
//		safe_assert(PIN_IsApplicationThread());
//		safe_assert(tid != INVALID_THREADID);
//		safe_assert(tid == PIN_ThreadId());
//	}
//
//	~ThreadLocalState() {
//		log_file << "Deleting thread local state for tid " << tid_ << endl;
//	}
//
//private:
//	DECL_FIELD(THREADID, tid)
//	DECL_FIELD_REF(CallStackType, call_stack)
//	DECL_FIELD(bool, inst_enabled)
//};

/* ===================================================================== */

LOCALVAR bool ThreadLocalState_inst_enabled[PIN_MAX_THREADS];
LOCALVAR CallStackType* ThreadLocalState_call_stack[PIN_MAX_THREADS];

/* ===================================================================== */

class PinSourceLocation;

typedef concurrit::ConcurrentMap<ADDRINT,PinSourceLocation*> AddrToLocMap;

class PinSourceLocation : public concurrit::SourceLocation {
public:

	static inline PinSourceLocation* get(VOID* address) {
		return PinSourceLocation::get(reinterpret_cast<ADDRINT>(address));
	}

	static inline PinSourceLocation* get(ADDRINT address) {
		PIN_LockClient();
		RTN rtn = RTN_FindByAddress(address);
		PinSourceLocation* loc = PinSourceLocation::get(rtn, address);
		PIN_UnlockClient();
		return loc;
	}

	static inline PinSourceLocation* get(RTN rtn) {
		return PinSourceLocation::get(rtn, RTN_Address(rtn));
	}

	static inline PinSourceLocation* get(RTN rtn, ADDRINT address) {
		PinSourceLocation* loc = NULL;
		if(addrToLoc_.find(address, &loc)) {
			safe_assert(loc != NULL);
		} else {
			loc = new PinSourceLocation(rtn, address);
			addrToLoc_.insert(address, loc);
		}
		safe_assert(loc != NULL);
		return loc;
	}

	static void clear() {
		for(AddrToLocMap::iterator itr = addrToLoc_.begin(); itr != addrToLoc_.end(); ++itr) {
			safe_assert(itr->second != NULL);
			delete itr->second;
		}
		addrToLoc_.clear();
	}

	PinSourceLocation(RTN rtn, ADDRINT address)
	: SourceLocation("<unknown>", "<unknown>", -1, -1, "<unknown>")
	{
		address_ = address;

		PIN_LockClient();

		PIN_GetSourceLocation(address_, &column_, &line_, &filename_);

		if(filename_ == "") {
			filename_ = "<unknown>";
		}

		if (RTN_Valid(rtn)) {
			IMG img = SEC_Img(RTN_Sec(rtn));
			if(IMG_Valid(img)) {
				imgname_ = IMG_Name(img);
			}
			funcname_ = RTN_Name(rtn);
		}

		PIN_UnlockClient();
	}

	VOID* pointer() {
		return reinterpret_cast<VOID*>(address_);
	}

	private:
	DECL_FIELD(ADDRINT, address)

	static AddrToLocMap addrToLoc_;
};

AddrToLocMap PinSourceLocation::addrToLoc_;

/* ===================================================================== */

typedef void (*NativePinMonitorFunType)(concurrit::EventBuffer*);
static AFUNPTR NativePinMonitorFunPtr = NULL;
static const char* NativePinMonitorFunName = "CallPinMonitor"; // _ZN9concurrit14CallPinMonitorEPNS_18PinMonitorCallInfoE

LOCALFUN INLINE
VOID CallNativePinMonitor(const CONTEXT * ctxt, THREADID tid, concurrit::EventBuffer* info) {
	if(NativePinMonitorFunPtr != NULL) {

//		reinterpret_cast<NativePinMonitorFunType>(NativePinMonitorFunPtr)(info);
		PIN_CallApplicationFunction(ctxt, tid,
			CALLINGSTD_DEFAULT, AFUNPTR(NativePinMonitorFunPtr),
			PIN_PARG(void), PIN_PARG(concurrit::EventBuffer*), (info), PIN_PARG_END());
	}
}

/* ===================================================================== */

//LOCALFUN ThreadLocalState* GetThreadLocalState(THREADID tid);

typedef std::set< std::string > RTNNamesToInstrumentType;
typedef concurrit::ConcurrentMap< ADDRINT, BOOL > RTNIdsToInstrumentType;

typedef std::set< std::string > RTNNamesToSkipType;
typedef concurrit::ConcurrentMap< ADDRINT, BOOL > RTNIdsToSkipType;

class InstParams {
private:
	InstParams() {}
	~InstParams() {}
public:
	static void ParseFile(const char* filename) {
		safe_assert(filename != NULL);

		RTNIdsToInstrument.insert(ADDRINT(0), TRUE); // dummy id

		RTNNamesToInstrument.insert("StartEndInstrument"); // dummy name

		std::vector<std::string> lines;
		concurrit::ReadLinesFromFile(filename, &lines, false, '#');
		for(std::vector<std::string>::iterator itr = lines.begin(); itr < lines.end(); ++itr) {
			std::string& s = (*itr);
			if(s.at(0) == '-') {
				s.erase(0,1);
				log_file << "Adding function to skip: " << (s) << std::endl;
				RTNNamesToSkip.insert(s);
			} else if (s == "All") {
				InstrumentAllRoutines = true;
				log_file << "Detected All in finfile.txt. Instrumenting all routines" << std::endl;
			} else {
				log_file << "Adding function to instrument: " << (s) << std::endl;
				RTNNamesToInstrument.insert(s);
			}
		}
	}

	static inline bool CheckAndRecordRoutine(const RTN& rtn, const ADDRINT& addr) {
		const std::string& name = RTN_Name(rtn);
		if(RTNNamesToInstrument.find(name) != RTNNamesToInstrument.end()) {
			// found
			log_file << "Detected routine to instrument: " << name << " at address " << addr << std::endl;
			RTNIdsToInstrument.insert(addr, TRUE);

			// communicate the access to the pin monitor in concurrit
			safe_check(name.size() < 64);
			concurrit::EventBuffer info;
			info.type = concurrit::AddressOfSymbol;
			info.threadid = 0;
			info.addr = addr;
			strncpy(info.str, name.c_str(), 63);

			reinterpret_cast<NativePinMonitorFunType>(NativePinMonitorFunPtr)(&info);
//			CallNativePinMonitor(ctxt, info->threadid, &info);

			return true;
		} else
		if(RTNNamesToSkip.find(name) != RTNNamesToSkip.end()) {
			// found
			log_file << "Detected routine to skip: " << name << " at address " << addr << std::endl;
			RTNIdsToSkip.insert(addr, TRUE);
			return true;
		}
		return false;
	}

	static inline bool IsRoutineToInstrument(const ADDRINT& rtn_addr) {
		if (rtn_addr == ADDRINT(0)) return true;
		if(InstrumentAllRoutines) return true;
		return (RTNIdsToInstrument.contains(rtn_addr));
	}

	static inline bool IsRoutineToSkip(const ADDRINT& rtn_addr) {
		if (rtn_addr == ADDRINT(0)) return true;
		return (RTNIdsToSkip.contains(rtn_addr));
	}

	static inline bool OnFuncEnter(const THREADID& threadid, const ADDRINT& rtn_addr) {
		if(!pin_enabled) return false;

		if(IsPLT(rtn_addr)) return false;

		// check callstack of current thread
//		ThreadLocalState* tls = GetThreadLocalState(threadid);
//		safe_assert(tls != NULL && tls->tid() == threadid);
		const bool is_inst_enabled = ThreadLocalState_inst_enabled[threadid]; // tls->inst_enabled();
		const bool is_inst_start = IsRoutineToInstrument(rtn_addr);
		if(is_inst_start) {

			// reset thread-local data structures
			if(!is_inst_enabled) {
				OnThreadRestart(threadid, true);
			}

			CallStackType* call_stack = ThreadLocalState_call_stack[threadid]; // tls->call_stack();
			safe_assert(call_stack != NULL);
			call_stack->push_back(rtn_addr);

			return true;
		}
		return is_inst_enabled;
	}

	static inline bool OnFuncReturn(const THREADID& threadid, const ADDRINT& rtn_addr) {
		if(!pin_enabled) return false;

		if(IsPLT(rtn_addr)) return false;

		// check callstack of current thread
//		ThreadLocalState* tls = GetThreadLocalState(threadid);
//		safe_assert(tls != NULL && tls->tid() == threadid);
		const bool is_inst_enabled = ThreadLocalState_inst_enabled[threadid]; // tls->inst_enabled();
		if(is_inst_enabled) {
			CallStackType* call_stack = ThreadLocalState_call_stack[threadid]; // tls->call_stack();
			safe_assert(call_stack != NULL);
			safe_assert(!call_stack->empty());
			const bool is_inst_start = IsRoutineToInstrument(rtn_addr);
			if(is_inst_start) {
				ADDRINT last_addr = call_stack->back(); call_stack->pop_back();
				if(rtn_addr != last_addr) {
					PIN_LockClient();
					safe_fail("Addresses %lu and %lu are not equal, %s, %s!\n", rtn_addr, last_addr, RTN_Name(RTN_FindByAddress(rtn_addr)).c_str(), RTN_Name(RTN_FindByAddress(last_addr)).c_str());
					PIN_UnlockClient();
				}
				safe_assert(rtn_addr == last_addr);

				ThreadLocalState_inst_enabled[threadid] = (!call_stack->empty()); // tls->set_inst_enabled(!call_stack->empty());
			}
			return true;
		}
		return false;
	}

	static inline bool OnInstruction(const THREADID& threadid) {
		if(!pin_enabled) return false;

		// check callstack of current thread
//		ThreadLocalState* tls = GetThreadLocalState(threadid);
//		safe_assert(tls != NULL && tls->tid() == threadid);
		return ThreadLocalState_inst_enabled[threadid]; // (tls->inst_enabled());
	}

	static inline void OnThreadRestart(const THREADID& threadid, bool enable = false) {
		// check callstack of current thread
//		ThreadLocalState* tls = GetThreadLocalState(threadid);
//		safe_assert(tls != NULL && tls->tid() == threadid);
		CallStackType* call_stack = ThreadLocalState_call_stack[threadid];
		safe_assert(call_stack != NULL);
		call_stack->clear(); // tls->call_stack()->clear();
		ThreadLocalState_inst_enabled[threadid] = enable; // tls->set_inst_enabled(enable);
	}

	static RTNNamesToInstrumentType RTNNamesToInstrument;
	static RTNIdsToInstrumentType RTNIdsToInstrument;
	static RTNNamesToSkipType RTNNamesToSkip;
	static RTNIdsToSkipType RTNIdsToSkip;
	static bool InstrumentAllRoutines;
	static volatile bool pin_enabled;
};


RTNNamesToInstrumentType InstParams::RTNNamesToInstrument;
RTNIdsToInstrumentType InstParams::RTNIdsToInstrument;
RTNNamesToSkipType InstParams::RTNNamesToSkip;
RTNIdsToSkipType InstParams::RTNIdsToSkip;
bool InstParams::InstrumentAllRoutines = false;
volatile bool InstParams::pin_enabled = true;

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
PinEnable() {
	InstParams::pin_enabled = true;
}

VOID PIN_FAST_ANALYSIS_CALL
PinDisable() {
	InstParams::pin_enabled = false;
}

VOID PIN_FAST_ANALYSIS_CALL
ThreadRestart(THREADID threadid) {
	InstParams::OnThreadRestart(threadid);
}

VOID PIN_FAST_ANALYSIS_CALL
StartInstrument(THREADID threadid) {
	safe_assert(!IsPLT(ADDRINT(0)));
	InstParams::OnFuncEnter(threadid, ADDRINT(0));
}

VOID PIN_FAST_ANALYSIS_CALL
EndInstrument(THREADID threadid) {
	safe_assert(!IsPLT(ADDRINT(0)));
	InstParams::OnFuncReturn(threadid, ADDRINT(0));
}

VOID PIN_FAST_ANALYSIS_CALL
PinToolShutdown() {
	PinDisable();
	PIN_Detach();
	PinSourceLocation::clear();
}

VOID PIN_FAST_ANALYSIS_CALL
PinToolInit(ADDRINT options_addrint) {
	concurrit::PinToolOptions* options_ptr = static_cast<concurrit::PinToolOptions*>(concurrit::ADDRINT2PTR(options_addrint));
	PIN_SafeCopy(&OPTIONS, options_ptr, sizeof(concurrit::PinToolOptions));

	log_file << "[PinToolInit] Pin Option TrackFuncCalls: " << OPTIONS.TrackFuncCalls << std::endl;
	log_file << "[PinToolInit] Pin Option InstrTopLevelFuncs: " << OPTIONS.InstrTopLevelFuncs << std::endl;
	log_file << "[PinToolInit] Pin Option InstrAfterMemoryAccess: " << OPTIONS.InstrAfterMemoryAccess << std::endl;
}

/* ===================================================================== */

#define BOOL2ADDRINT(b)	((b) ? ADDRINT(1) : ADDRINT(0))

ADDRINT PIN_FAST_ANALYSIS_CALL
If_OnFuncEnter(THREADID threadid, ADDRINT rtn_addr) {
	return BOOL2ADDRINT(InstParams::OnFuncEnter(threadid, rtn_addr));
}

ADDRINT PIN_FAST_ANALYSIS_CALL
If_OnFuncReturn(THREADID threadid, ADDRINT rtn_addr) {
	return BOOL2ADDRINT(InstParams::OnFuncReturn(threadid, rtn_addr));
}

ADDRINT PIN_FAST_ANALYSIS_CALL
If_OnInstruction(THREADID threadid) {
	return BOOL2ADDRINT(InstParams::OnInstruction(threadid));
}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
FuncCall(const CONTEXT * ctxt, THREADID threadid, BOOL direct, PinSourceLocation* loc_src,
		ADDRINT target, ADDRINT arg0, ADDRINT arg1, ADDRINT rtn_addr) {

//	if(!InstParams::OnInstruction(threadid)) {
//		return;
//	}
	//===============================================================

	PinSourceLocation* loc_target = PinSourceLocation::get(target);

	concurrit::EventBuffer info;
	info.type = concurrit::FuncCall;
	info.threadid = threadid;
	info.addr = rtn_addr; // loc_src->pointer();
	info.addr_target = target; // loc_target->pointer();
	info.direct = direct;
	info.loc_src = loc_src;
	info.loc_target = loc_target;
	info.arg0 = arg0;
	info.arg1 = arg1;

	CallNativePinMonitor(ctxt, threadid, &info);
}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
FuncEnter(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc,
		ADDRINT arg0, ADDRINT arg1) {

//	log_file << threadid << " entering " << loc->funcname() << std::endl;

//	if(!InstParams::OnFuncEnter(threadid, concurrit::PTR2ADDRINT(loc->pointer()))) {
//		return;
//	}

	//=======================================

	concurrit::EventBuffer info;
	info.type = concurrit::FuncEnter;
	info.threadid = threadid;
	info.addr = concurrit::PTR2ADDRINT(loc->pointer());
	info.loc_src = loc;
	info.arg0 = arg0;
	info.arg1 = arg1;

	CallNativePinMonitor(ctxt, threadid, &info);
}

VOID PIN_FAST_ANALYSIS_CALL
FuncEnterEx(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc,
		ADDRINT arg0, ADDRINT arg1) {

//	log_file << threadid << " entering " << loc->funcname() << std::endl;

	if(!InstParams::OnFuncEnter(threadid, concurrit::PTR2ADDRINT(loc->pointer()))) return;

	//=======================================

	concurrit::EventBuffer info;
	info.type = concurrit::FuncEnter;
	info.threadid = threadid;
	info.addr = concurrit::PTR2ADDRINT(loc->pointer());
	info.loc_src = loc;
	info.arg0 = arg0;
	info.arg1 = arg1;

	CallNativePinMonitor(ctxt, threadid, &info);
}

VOID PIN_FAST_ANALYSIS_CALL
FuncReturn(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc, ADDRINT ret0, ADDRINT rtn_addr) {

//	log_file << threadid << " returning " << loc->funcname() << std::endl;

//	if(!InstParams::OnFuncReturn(threadid, rtn_addr)) {
//		return;
//	}

	//=======================================

	concurrit::EventBuffer info;
	info.type = concurrit::FuncReturn;
	info.threadid = threadid;
	info.addr = rtn_addr; // loc_src->pointer();
	info.loc_src = loc;
	info.retval = ret0;

	CallNativePinMonitor(ctxt, threadid, &info);
}

/* ===================================================================== */

//struct AddrSizePair {
//	VOID * addr;
//	UINT32 size;
//};
//
//LOCALVAR AddrSizePair AddrSizePairs[PIN_MAX_THREADS];
//
//INLINE VOID CaptureAddrSize(THREADID threadid, VOID * addr, UINT32 size) {
//	AddrSizePairs[threadid].addr = addr;
//	AddrSizePairs[threadid].size = size;
//}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
MemAccessBefore(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc) {

//	if(!InstParams::OnInstruction(threadid)) {
//		return;
//	}
	//===============================================================

//	CaptureAddrSize(threadid, addr, size);

	concurrit::EventBuffer info;
	info.type = concurrit::MemAccessBefore;
	info.threadid = threadid;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
}

VOID PIN_FAST_ANALYSIS_CALL
MemAccessAfter(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc) {

//	if(!InstParams::OnInstruction(threadid)) {
//		return;
//	}
	//===============================================================

//	VOID * addr = AddrSizePairs[threadid].addr;
//	UINT32 size = AddrSizePairs[threadid].size;

	concurrit::EventBuffer info;
	info.type = concurrit::MemAccessAfter;
	info.threadid = threadid;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
MemWrite(const CONTEXT * ctxt, THREADID threadid, VOID * addr, UINT32 size, PinSourceLocation* loc) {

//	if(!InstParams::OnInstruction(threadid)) {
//		return;
//	}
	//===============================================================

//	CaptureAddrSize(threadid, addr, size);

	concurrit::EventBuffer info;
	info.type = concurrit::MemWrite;
	info.threadid = threadid;
	info.addr = concurrit::PTR2ADDRINT(addr);
	info.size = size;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
}

VOID PIN_FAST_ANALYSIS_CALL
MemRead(const CONTEXT * ctxt, THREADID threadid, VOID * addr, UINT32 size, PinSourceLocation* loc) {

//	if(!InstParams::OnInstruction(threadid)) {
//		return;
//	}
	//===============================================================

//	CaptureAddrSize(threadid, addr, size);

	concurrit::EventBuffer info;
	info.type = concurrit::MemRead;
	info.threadid = threadid;
	info.addr = concurrit::PTR2ADDRINT(addr);
	info.size = size;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
}

/* ===================================================================== */

LOCALVAR std::vector<string> FilteredImages;
typedef concurrit::ConcurrentMap<UINT32,BOOL> FilteredImageIdsType;
LOCALVAR FilteredImageIdsType FilteredImageIds;

LOCALFUN INLINE
void InitFilteredImages(const char* filename) {
	safe_assert(filename != NULL);

	FilteredImages.clear();
	concurrit::ReadLinesFromFile(filename, &FilteredImages, false, '#');
}

/* ===================================================================== */

LOCALVAR volatile bool is_concurrit_loaded = false;

LOCALFUN INLINE
VOID OnLoadConcurrit(IMG img) {
	safe_assert(!is_concurrit_loaded);

	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
	{
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
		{
			if(RTN_Name(rtn) == NativePinMonitorFunName) {
				RTN_Open(rtn);

				NativePinMonitorFunPtr = RTN_Funptr(rtn);

				log_file << "Detected callback to concurrit: " << NativePinMonitorFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);

			} else if(RTN_Name(rtn) == NativePinEnableFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinEnable), IARG_FAST_ANALYSIS_CALL,
						IARG_END);

				log_file << "Detected callback to concurrit: " << NativePinEnableFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);

			}  else if(RTN_Name(rtn) == NativePinDisableFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinDisable), IARG_FAST_ANALYSIS_CALL,
						IARG_END);

				log_file << "Detected callback to concurrit: " << NativePinDisableFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);

			} else if(RTN_Name(rtn) == NativeThreadRestartFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(ThreadRestart), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);

				log_file << "Detected callback to concurrit: " << NativeThreadRestartFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);

			} else if(RTN_Name(rtn) == NativeStartInstrumentFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(StartInstrument), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);

				log_file << "Detected callback to concurrit: " << NativeStartInstrumentFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);

			} else if(RTN_Name(rtn) == NativeEndInstrumentFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(EndInstrument), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);

				log_file << "Detected callback to concurrit: " << NativeEndInstrumentFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);

			} else if(RTN_Name(rtn) == NativePinShutdownFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinToolShutdown), IARG_FAST_ANALYSIS_CALL,
						IARG_END);

				log_file << "Detected callback to concurrit: " << NativePinShutdownFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);
			} else if(RTN_Name(rtn) == NativePinInitFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinToolInit), IARG_FAST_ANALYSIS_CALL,
						IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
						IARG_END);

				log_file << "Detected callback to concurrit: " << NativePinInitFunName << " at address " << RTN_Address(rtn) << std::endl;

				RTN_Close(rtn);
			}
		}
	}
	if(NativePinMonitorFunPtr == NULL) {
		fprintf(stderr, "Could not find callback to concurrit: %s\n", NativePinMonitorFunName);
		_Exit(UNRECOVERABLE_ERROR);
	}

	is_concurrit_loaded = true;
}

/* ===================================================================== */

LOCALFUN INLINE
BOOL IsImageFiltered(IMG img) {
	if(IMG_IsMainExecutable(img)) {
		return TRUE;
	}

	UINT32 img_id = IMG_Id(img);

	BOOL value = TRUE;
	if(FilteredImageIds.find(img_id, &value)) {
		safe_assert(value == TRUE || value == FALSE);
		return value;
	}

	string img_name = IMG_Name(img);
	for(std::vector<string>::iterator itr = FilteredImages.begin(); itr < FilteredImages.end(); ++itr) {
		if(img_name.find(*itr) != string::npos) {

			log_file << "--- IMG --- " << IMG_Name(img) << std::endl;
			FilteredImageIds.insert(img_id, TRUE);

			/* ================================= */
			if (!is_concurrit_loaded && *itr == "libconcurrit.so") {
				OnLoadConcurrit(img);
			}
			/* ================================= */

			return TRUE;
		}
	}

	log_file << "+++ IMG +++ " << IMG_Name(img) << std::endl;
	FilteredImageIds.insert(img_id, FALSE);
	return FALSE;
}

/* ===================================================================== */

LOCALFUN INLINE
BOOL IsRoutineFiltered(RTN rtn, ADDRINT rtn_addr) {
	if(RTN_Valid(rtn)) {
		// be careful to always call IsImageFiltered
		return IsImageFiltered(SEC_Img(RTN_Sec(rtn)))
				|| InstParams::IsRoutineToSkip(rtn_addr)
				|| (OPTIONS.InstrTopLevelFuncs && !InstParams::IsRoutineToInstrument(rtn_addr));
	}
	return TRUE;
}

/* ===================================================================== */

LOCALFUN INLINE
VOID CallTrace(INS ins, RTN rtn, ADDRINT rtn_addr) {

	if (OPTIONS.TrackFuncCalls && INS_IsCall(ins) && INS_IsProcedureCall(ins) && !INS_IsSyscall(ins)) {
		if (!INS_IsDirectBranchOrCall(ins)) {
			// Indirect call
			PinSourceLocation* loc = PinSourceLocation::get(rtn, INS_Address(ins));

			INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
							IARG_THREAD_ID, IARG_END);

			INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(FuncCall), IARG_FAST_ANALYSIS_CALL,
					IARG_CONTEXT,
					IARG_THREAD_ID, IARG_BOOL, FALSE, IARG_PTR, loc,
					IARG_BRANCH_TARGET_ADDR,
					IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_FUNCARG_CALLSITE_VALUE, 1, IARG_ADDRINT, rtn_addr, IARG_END);

		} else if (INS_IsDirectBranchOrCall(ins)) {
			// Direct call
			PinSourceLocation* loc = PinSourceLocation::get(rtn, INS_Address(ins));

			ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);

			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncCall), IARG_FAST_ANALYSIS_CALL,
					IARG_CONTEXT,
					IARG_THREAD_ID, IARG_PTR, TRUE, IARG_PTR, loc,
					IARG_ADDRINT, target,
					IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_FUNCARG_CALLSITE_VALUE, 1, IARG_ADDRINT, rtn_addr, IARG_END);

		}
	} else
	if (INS_IsRet(ins) && !INS_IsSysret(ins)) {
#if defined(TARGET_LINUX) && defined(TARGET_IA32)
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_runtime_resolve") return;
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_debug_state") return;
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_fixup") return;
#endif
		PinSourceLocation* loc = PinSourceLocation::get(rtn, INS_Address(ins));

		if(InstParams::IsRoutineToInstrument(rtn_addr)) {
			INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnFuncReturn), IARG_FAST_ANALYSIS_CALL,
					IARG_THREAD_ID, IARG_ADDRINT, rtn_addr, IARG_END);
		} else {
			INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
					IARG_THREAD_ID, IARG_END);
		}

		INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(FuncReturn), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_PTR, loc, IARG_FUNCRET_EXITPOINT_VALUE, IARG_ADDRINT, rtn_addr, IARG_END);
	}
}

/* ===================================================================== */

LOCALFUN INLINE
VOID MemoryTrace(INS ins, RTN rtn) {
	if (INS_IsStackRead(ins) || INS_IsStackWrite(ins)) return;

	bool is_access = INS_IsMemoryWrite(ins) || INS_HasMemoryRead2(ins) || (INS_IsMemoryRead(ins) && !INS_IsPrefetch(ins));
	if(!is_access) return;
	bool has_fallthrough = false;
	bool is_branchorcall = false;
	if(OPTIONS.InstrAfterMemoryAccess) {
		has_fallthrough = INS_HasFallThrough(ins);
		is_branchorcall = INS_IsBranchOrCall(ins);
		if(!has_fallthrough && !is_branchorcall) return;
	}

	PinSourceLocation* loc = PinSourceLocation::get(rtn, INS_Address(ins));

	/* ==================== */

	if (INS_IsMemoryWrite(ins)) {
		INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

		INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemWrite), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_PTR, loc, IARG_END);
	}

	/* ==================== */

	if (INS_HasMemoryRead2(ins)) {
		INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

		INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemRead), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_PTR, loc, IARG_END);
	}

	/* ==================== */

	if (INS_IsMemoryRead(ins) && !INS_IsPrefetch(ins)) {
		INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

		INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemRead), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_PTR, loc, IARG_END);
	}

	/* ======================================== */

	INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
			IARG_THREAD_ID, IARG_END);

	INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemAccessBefore), IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);

	/* ==================== */

	if(OPTIONS.InstrAfterMemoryAccess) {
		if (has_fallthrough) {
			INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

			INS_InsertThenPredicatedCall(ins, IPOINT_AFTER, AFUNPTR(MemAccessAfter), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_PTR, loc, IARG_END);
		}
		if (is_branchorcall) {
			INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

			INS_InsertThenPredicatedCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(MemAccessAfter), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_PTR, loc, IARG_END);
		}
	}
}

/* ===================================================================== */

LOCALFUN INLINE
VOID Instruction(INS ins, RTN rtn, ADDRINT rtn_addr) {
	if (INS_IsOriginal(ins)) {

		MemoryTrace(ins, rtn);

		CallTrace(ins, rtn, rtn_addr);
	}
}

/* ===================================================================== */

LOCALFUN
VOID Routine(RTN rtn, VOID *v) {

	if(!RTN_Valid(rtn)) return;

	ADDRINT rtn_addr = RTN_Address(rtn);

	// if this is a routine to forward to concurrit, then record its address
	InstParams::CheckAndRecordRoutine(rtn, rtn_addr);

	// filter out standard libraries
	// we treat this while loading, since Routine can be called before ImageLoad is called
	if(IsRoutineFiltered(rtn, rtn_addr)) return;

//	PIN_LockClient();

	RTN_Open(rtn);

	// check plt routine
	if(RTN_Name(rtn) == ".plt") {
		AddToPLT(rtn_addr);
		RTN_Close(rtn);
		return;
	}

	PinSourceLocation* loc = PinSourceLocation::get(rtn);
	safe_assert(concurrit::PTR2ADDRINT(loc->pointer()) == rtn_addr);

	log_file << "+++ RTN +++ " << RTN_Name(rtn) << " at address " << rtn_addr << std::endl;

	// check if start/end instrument
	if(RTN_Name(rtn) == NativeStartInstrumentFunName) {
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(StartInstrument), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

	} else if(RTN_Name(rtn) == NativeEndInstrumentFunName) {
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(EndInstrument), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);
	} else {
		// standard instrumentation
		INS ins = RTN_InsHeadOnly(rtn);
		if(INS_Valid(ins)) {

			if(InstParams::IsRoutineToInstrument(rtn_addr)) {
				INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnFuncEnter), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_ADDRINT, rtn_addr, IARG_END);
			} else {
				INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(If_OnInstruction), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);
			}

			INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(FuncEnter), IARG_FAST_ANALYSIS_CALL,
				   IARG_CONTEXT,
				   IARG_THREAD_ID, IARG_PTR, loc,
				   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
				   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
				   IARG_END);

		} else {

			RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(FuncEnterEx), IARG_FAST_ANALYSIS_CALL,
						   IARG_CONTEXT,
						   IARG_THREAD_ID, IARG_PTR, loc,
						   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
						   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
						   IARG_END);
		}
	}

	RTN_Close(rtn);

//	PIN_UnlockClient();
}

/* ===================================================================== */

LOCALFUN
VOID ImageLoad(IMG img, VOID *) {

	// filter out standard libraries
	// also updates filtered image ids
	if(IsImageFiltered(img)) return;

//	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
//	{
//		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
//		{
//			log_file << "\t+++ RTN +++ " << RTN_Name(rtn) << endl;
//
//			RTN_Open(rtn);
//
//			PinSourceLocation* loc = PinSourceLocation::get(rtn);
//
//			RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(FuncEnter), IARG_FAST_ANALYSIS_CALL,
//					   IARG_CONTEXT,
//					   IARG_THREAD_ID, IARG_PTR, loc,
//					   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
//					   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
//					   IARG_END);
//
//			RTN_Close(rtn);
//		}
//	}
}

/* ===================================================================== */

LOCALFUN
VOID ImageUnload(IMG img, VOID *) {

	log_file << "Unloading image: " << IMG_Name(img) << std::endl;

	// delete filtering info about this image
	UINT32 img_id = IMG_Id(img);
	FilteredImageIds.erase(img_id);
}

/* ===================================================================== */

LOCALFUN
VOID Trace(TRACE trace, VOID *v) {

	if (!filter.SelectTrace(trace)) return;

	RTN rtn = TRACE_Rtn(trace);
	if(!RTN_Valid(rtn)) return;

	ADDRINT rtn_addr = RTN_Address(rtn);
	if (IsRoutineFiltered(rtn, rtn_addr)) return;

	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
	{
		for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
		{
			Instruction(ins, rtn, rtn_addr);
		}
	}
}

/* ===================================================================== */

LOCALFUN
VOID Fini(INT32 code, VOID *v) {
	timer.stop();

	log_file << std::endl << "PIN-TIME for program: " << timer.ElapsedTimeToString() << std::endl;

	PIN_DeleteThreadDataKey(tls_key);

	log_file.close();
}

/* ===================================================================== */

LOCALFUN
VOID Detach(VOID *v) {
	log_file << "Detaching Pintool." << std::endl;

	Fini(EXIT_SUCCESS, v);
}

/* ===================================================================== */

//LOCALFUN void OnSig(THREADID threadIndex, CONTEXT_CHANGE_REASON reason,
//		const CONTEXT *ctxtFrom, CONTEXT *ctxtTo, INT32 sig, VOID *v) {
//	if (ctxtFrom != 0) {
//		ADDRINT address = PIN_GetContextReg(ctxtFrom, REG_INST_PTR);
//		cerr << "SIG signal=" << sig << " on thread " << threadIndex
//				<< " at address " << hex << address << dec << " ";
//	}
//
//	switch (reason) {
//	case CONTEXT_CHANGE_REASON_FATALSIGNAL:
//		cerr << "FATALSIG" << sig;
//		break;
//	case CONTEXT_CHANGE_REASON_SIGNAL:
//		cerr << "SIGNAL " << sig;
//		break;
//	case CONTEXT_CHANGE_REASON_SIGRETURN:
//		cerr << "SIGRET";
//		break;
//
//	case CONTEXT_CHANGE_REASON_APC:
//		cerr << "APC";
//		break;
//
//	case CONTEXT_CHANGE_REASON_EXCEPTION:
//		cerr << "EXCEPTION";
//		break;
//
//	case CONTEXT_CHANGE_REASON_CALLBACK:
//		cerr << "CALLBACK";
//		break;
//
//	default:
//		break;
//	}
//	cerr << std::endl;
//}

/* ===================================================================== */

//LOCALFUN ThreadLocalState* GetThreadLocalState(THREADID tid) {
//	safe_assert(tid == PIN_ThreadId());
//	VOID* ptr = PIN_GetThreadData(tls_key, tid);
//	safe_assert(ptr != NULL);
//	ThreadLocalState* tls = static_cast<ThreadLocalState*>(ptr);
//	safe_assert(tls->tid() == tid);
//	return tls;
//}

LOCALFUN VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags,
		VOID *v) {
	safe_assert(threadid == PIN_ThreadId());
	log_file << "Thread "<< threadid << " starting..." << endl;

//	GLB_LOCK();
//	PIN_SetThreadData(tls_key, new ThreadLocalState(threadid), threadid);
//	GLB_UNLOCK();

	safe_assert(BETWEEN(0, threadid, PIN_MAX_THREADS));
	ThreadLocalState_inst_enabled[threadid] = false;
	ThreadLocalState_call_stack[threadid] = new CallStackType();
}

// This routine is executed every time a thread is destroyed.
LOCALFUN VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code,
		VOID *v) {
	safe_assert(threadid == PIN_ThreadId());

	log_file << "Thread "<< threadid << " ending..." << endl;

	safe_assert(BETWEEN(0, threadid, PIN_MAX_THREADS));
	CallStackType* call_stack = ThreadLocalState_call_stack[threadid];
	safe_assert(call_stack != NULL);
	delete call_stack;
}

//LOCALFUN VOID ThreadLocalDestruct(VOID* ptr) {
//	if (ptr != NULL) {
//		ThreadLocalState* o = static_cast<ThreadLocalState*>(ptr);
//		delete o;
//	}
//}

/* ===================================================================== */

int main(int argc, CHAR *argv[]) {
	PIN_InitSymbols();

	if (PIN_Init(argc, argv)) {
		return Usage();
	}

	CONCURRIT_HOME = std::string(getenv("CONCURRIT_HOME"));

//	control.CheckKnobs(Handler, 0);
	skipper.CheckKnobs(0);

	log_file.open(KnobLogFile.Value().c_str());

	// read function names to instrument
	InstParams::ParseFile(KnobFinFile.Value().c_str());

	InitFilteredImages(KnobFilteredImagesFile.Value().c_str());

	OPTIONS.InstrTopLevelFuncs = KnobInstrumentTopLevel.Value();
	OPTIONS.TrackFuncCalls = KnobTrackFuncCalls.Value();

	log_file << "[main] Pin Option TrackFuncCalls: " << OPTIONS.TrackFuncCalls << std::endl;
	log_file << "[main] Pin Option InstrTopLevelFuncs: " << OPTIONS.InstrTopLevelFuncs << std::endl;

//	tls_key = PIN_CreateThreadDataKey(ThreadLocalDestruct);
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	PIN_AddFiniFunction(Fini, 0);
	PIN_AddDetachFunction(Detach, 0);

	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);

	RTN_AddInstrumentFunction(Routine, 0);

//#if !INSTR_ALL_TRACES
	TRACE_AddInstrumentFunction(Trace, 0);
//#endif

//	PIN_AddContextChangeFunction(OnSig, 0);

	filter.Activate();

	// GLB_LOCK_INIT(); // uncomment if needed

	// start the timer
	timer.start();

	// Never returns

	PIN_StartProgram();

	return 0;
}

/* ===================================================================== */
