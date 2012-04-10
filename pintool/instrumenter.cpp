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
#include "statistics.h"
#include "sharedaccess.h"
#include "pinmonitor.h"
#include "tbb/concurrent_hash_map.h"

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobLogFile(KNOB_MODE_WRITEONCE, "pintool", "log_file",
		"~/concurrit/work/pinlogfile.txt", "Specify log file path");

KNOB<string> KnobFinFile(KNOB_MODE_WRITEONCE, "pintool", "finfile",
		"~/concurrit/work/finfile.txt", "Specify names of functions to instrument");

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

LOCALVAR PIN_LOCK lock;
#define GLB_LOCK_INIT()		InitLock(&lock)
#define GLB_LOCK()			GetLock(&lock, 1)
#define GLB_UNLOCK()		ReleaseLock(&lock)

/* ===================================================================== */

static const char* NativePinEnableFunName = "EnablePinTool";
static const char* NativePinDisableFunName = "DisablePinTool";
static const char* NativeThreadRestartFunName = "ThreadRestart";
static const char* NativeStartInstrumentFunName = "StartInstrument";
static const char* NativeEndInstrumentFunName = "EndInstrument";

VOID PIN_FAST_ANALYSIS_CALL
PinEnableDisable(UINT32 command);

/* ===================================================================== */

static ADDRINT rtn_plt_addr = 0;

/* ===================================================================== */

class ThreadLocalState {
public:
	ThreadLocalState(THREADID tid, bool inst_enabled = false) : tid_(tid), inst_enabled_(inst_enabled) {
		log_file << "Creating thread local state for tid " << tid_ << endl;

		safe_assert(PIN_IsApplicationThread());
		safe_assert(tid != INVALID_THREADID);
		safe_assert(tid == PIN_ThreadId());
	}

	~ThreadLocalState() {
		log_file << "Deleting thread local state for tid " << tid_ << endl;
	}

private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD_REF(std::vector<ADDRINT>, call_stack)
	DECL_FIELD(bool, inst_enabled)
};

/* ===================================================================== */

LOCALFUN ThreadLocalState* GetThreadLocalState(THREADID tid);

class InstParams {
private:
	InstParams() {}
	~InstParams() {}
public:
	static void ParseFile(const char* filename) {
		safe_assert(filename != NULL);
		FILE* fin = fopen(filename, "r");

		RTNIdsToInstrument.clear();
		RTNIdsToInstrument.insert(ADDRINT(0)); // dummy id

		RTNNamesToInstrument.clear();
		RTNNamesToInstrument.insert("StartEndInstrument"); // dummy name

		if (fin != NULL) {
			char buff[256];
			while(!feof(fin)) {
				if(fgets(buff, 256, fin) == NULL) {
					break;
				} else {
					size_t sz = strnlen(buff, 256);
					if(buff[sz-1] == '\n') {
						buff[sz-1] = '\0';
					}
					log_file << "Adding routine to instrument: " << buff << std::endl;
					RTNNamesToInstrument.insert(buff);
				}
			}
			fclose(fin);
		}
	}

	static inline void CheckAndRecordRoutine(const RTN& rtn, const ADDRINT& addr) {
		const std::string& name = RTN_Name(rtn);
		if(RTNNamesToInstrument.find(name) != RTNNamesToInstrument.end()) {
			// found
			RTNIdsToInstrument.insert(addr);
			log_file << "Detected routine to instrument: " << name << std::endl;
		}
	}

	static inline bool OnFuncEnter(const THREADID& threadid, const ADDRINT& rtn_addr) {
		if(!pin_enabled) return false;

		safe_assert(rtn_addr != rtn_plt_addr);

		// check callstack of current thread
		ThreadLocalState* tls = GetThreadLocalState(threadid);
		safe_assert(tls != NULL && tls->tid() == threadid);
		if(tls->inst_enabled()
				|| (rtn_addr == 0) // StartInstrument
				|| (!RTNIdsToInstrument.empty() && RTNIdsToInstrument.find(rtn_addr) != RTNIdsToInstrument.end())) {
			std::vector<ADDRINT>* call_stack = tls->call_stack();
			call_stack->push_back(rtn_addr);
			tls->set_inst_enabled(true);
			return true;
		}
		return false;
	}

	static inline bool OnFuncReturn(const THREADID& threadid, const ADDRINT& rtn_addr) {
		if(!pin_enabled) return false;

		safe_assert(rtn_plt_addr != 0);
		if(rtn_addr == rtn_plt_addr) return false;

		// check callstack of current thread
		ThreadLocalState* tls = GetThreadLocalState(threadid);
		safe_assert(tls != NULL && tls->tid() == threadid);
		if(tls->inst_enabled()) {
			std::vector<ADDRINT>* call_stack = tls->call_stack();
			ADDRINT last_addr = call_stack->back(); call_stack->pop_back();
			safe_assert(rtn_addr == last_addr);

			safe_assert(rtn_addr != 0 || tls->inst_enabled());
			tls->set_inst_enabled(!call_stack->empty());
			safe_assert(rtn_addr != 0 || !tls->inst_enabled());
			return true;
		}
		safe_assert(RTNIdsToInstrument.empty() || RTNIdsToInstrument.find(rtn_addr) == RTNIdsToInstrument.end());
		return false;
	}

	static inline bool OnInstruction(const THREADID& threadid) {
		if(!pin_enabled) return false;

		// check callstack of current thread
		ThreadLocalState* tls = GetThreadLocalState(threadid);
		safe_assert(tls != NULL && tls->tid() == threadid);
		return (tls->inst_enabled());
	}

	static inline void OnThreadRestart(const THREADID& threadid) {
		// check callstack of current thread
		ThreadLocalState* tls = GetThreadLocalState(threadid);
		safe_assert(tls != NULL && tls->tid() == threadid);
		tls->call_stack()->clear();
		tls->set_inst_enabled(false);
	}

	static std::set< std::string > 	RTNNamesToInstrument;
	static std::set< ADDRINT > 		RTNIdsToInstrument;
	static volatile bool			pin_enabled;
};

std::set< std::string > InstParams::RTNNamesToInstrument;
std::set< ADDRINT > 	InstParams::RTNIdsToInstrument;
volatile bool			InstParams::pin_enabled = true;

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
	InstParams::OnFuncEnter(threadid, ADDRINT(0));
}

VOID PIN_FAST_ANALYSIS_CALL
EndInstrument(THREADID threadid) {
	InstParams::OnFuncReturn(threadid, ADDRINT(0));
}

/* ===================================================================== */

typedef void (*NativePinMonitorFunType)(concurrit::PinMonitorCallInfo*);
static AFUNPTR NativePinMonitorFunPtr = NULL;
static const char* NativePinMonitorFunName = "CallPinMonitor"; // _ZN9concurrit14CallPinMonitorEPNS_18PinMonitorCallInfoE

LOCALFUN VOID CallNativePinMonitor(const CONTEXT * ctxt, THREADID tid, concurrit::PinMonitorCallInfo* info) {
	if(NativePinMonitorFunPtr != NULL) {

//		reinterpret_cast<NativePinMonitorFunType>(NativePinMonitorFunPtr)(info);
		PIN_CallApplicationFunction(ctxt, tid,
			CALLINGSTD_DEFAULT, AFUNPTR(NativePinMonitorFunPtr),
			PIN_PARG(void), PIN_PARG(concurrit::PinMonitorCallInfo*), (info), PIN_PARG_END());
	}
}

/* ===================================================================== */

class PinSourceLocation;

typedef tbb::concurrent_hash_map<ADDRINT,PinSourceLocation*> AddrToLocMap;

class PinSourceLocation : public concurrit::SourceLocation {
public:

	static PinSourceLocation* get(VOID* address) {
		return PinSourceLocation::get(reinterpret_cast<ADDRINT>(address));
	}

	static PinSourceLocation* get(ADDRINT address) {
		PIN_LockClient();
		RTN rtn = RTN_FindByAddress(address);
		PIN_UnlockClient();
		return PinSourceLocation::get(rtn, address);
	}

	static PinSourceLocation* get(RTN rtn) {
		return PinSourceLocation::get(rtn, RTN_Address(rtn));
	}

	static PinSourceLocation* get(RTN rtn, ADDRINT address) {
		PinSourceLocation* loc = NULL;
		AddrToLocMap::accessor acc;
		if(addrToLoc_.find(acc, address)) {
			loc = acc->second;
			safe_assert(loc != NULL);
		} else {
			loc = new PinSourceLocation(rtn, address);
			addrToLoc_.insert(acc, address);
			acc->second = loc;
		}
		safe_assert(loc != NULL);
		return loc;
	}

	PinSourceLocation(RTN rtn, ADDRINT address)
	: SourceLocation("<unknown>", "<unknown>", -1, -1, "<unknown>")
	{
		address_ = address;

		PIN_LockClient();
		PIN_GetSourceLocation(address_, &column_, &line_, &filename_);
		PIN_UnlockClient();

		if(filename_ == "") {
			filename_ = "<unknown>";
		}

		if (RTN_Valid(rtn))
		{
			imgname_ = IMG_Name(SEC_Img(RTN_Sec(rtn)));
			funcname_ = RTN_Name(rtn);
		}
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

VOID PIN_FAST_ANALYSIS_CALL
FuncCall(const CONTEXT * ctxt, THREADID threadid, BOOL direct, PinSourceLocation* loc_src,
		ADDRINT target, ADDRINT arg0, ADDRINT arg1) {

	if(!InstParams::OnInstruction(threadid)) {
		return;
	}
	//===============================================================

	PinSourceLocation* loc_target = PinSourceLocation::get(target);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::FuncCall;
	info.threadid = threadid;
	info.addr = loc_src->pointer();
	info.addr_target = loc_target->pointer();
	info.direct = direct;
	info.loc_src = loc_src;
	info.loc_target = loc_target;
	info.arg0 = arg0;
	info.arg1 = arg1;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->FuncCall(threadid, loc_target->pointer(), direct, loc_src, loc_target);
}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
FuncEnter(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc,
		ADDRINT arg0, ADDRINT arg1) {

//	log_file << threadid << " entering " << loc->funcname() << std::endl;

	if(!InstParams::OnFuncEnter(threadid, PTR2ADDRINT(loc->pointer()))) {
		return;
	}

	//=======================================

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::FuncEnter;
	info.threadid = threadid;
	info.addr = loc->pointer();
	info.loc_src = loc;
	info.arg0 = arg0;
	info.arg1 = arg1;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->FuncEnter(threadid, loc->pointer(), loc);
}

VOID PIN_FAST_ANALYSIS_CALL
FuncReturn(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc, ADDRINT ret0, ADDRINT rtn_addr) {

//	log_file << threadid << " returning " << loc->funcname() << std::endl;

	if(!InstParams::OnFuncReturn(threadid, rtn_addr)) {
		return;
	}

	//=======================================

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::FuncReturn;
	info.threadid = threadid;
	info.addr = ADDRINT2PTR(rtn_addr);
	info.loc_src = loc;
	info.retval = ret0;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->FuncReturn(threadid, loc->pointer(), loc, ret0);
}

/* ===================================================================== */

struct AddrSizePair {
	VOID * addr;
	UINT32 size;
};

LOCALVAR AddrSizePair AddrSizePairs[PIN_MAX_THREADS];

INLINE VOID CaptureAddrSize(THREADID threadid, VOID * addr, UINT32 size) {
	AddrSizePairs[threadid].addr = addr;
	AddrSizePairs[threadid].size = size;
}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
MemAccessBefore(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc) {

	if(!InstParams::OnInstruction(threadid)) {
		return;
	}
	//===============================================================

//	CaptureAddrSize(threadid, addr, size);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemAccessBefore;
	info.threadid = threadid;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemWriteBefore(threadid, addr, size, loc);

}

VOID PIN_FAST_ANALYSIS_CALL
MemAccessAfter(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc) {

	if(!InstParams::OnInstruction(threadid)) {
		return;
	}
	//===============================================================

//	VOID * addr = AddrSizePairs[threadid].addr;
//	UINT32 size = AddrSizePairs[threadid].size;

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemAccessAfter;
	info.threadid = threadid;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemWriteAfter(threadid, addr, size, loc);
}

/* ===================================================================== */

VOID PIN_FAST_ANALYSIS_CALL
MemWrite(const CONTEXT * ctxt, THREADID threadid, VOID * addr, UINT32 size, PinSourceLocation* loc) {

	if(!InstParams::OnInstruction(threadid)) {
		return;
	}
	//===============================================================

	CaptureAddrSize(threadid, addr, size);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemWrite;
	info.threadid = threadid;
	info.addr = addr;
	info.size = size;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemReadBefore(threadid, addr, size, loc);
}

VOID PIN_FAST_ANALYSIS_CALL
MemRead(const CONTEXT * ctxt, THREADID threadid, VOID * addr, UINT32 size, PinSourceLocation* loc) {

	if(!InstParams::OnInstruction(threadid)) {
		return;
	}
	//===============================================================

	CaptureAddrSize(threadid, addr, size);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemRead;
	info.threadid = threadid;
	info.addr = addr;
	info.size = size;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemReadBefore(threadid, addr, size, loc);
}

/* ===================================================================== */

LOCALVAR std::vector<string> FilteredImages;
typedef tbb::concurrent_hash_map<UINT32,BOOL> FilteredImageIdsType;
LOCALVAR FilteredImageIdsType FilteredImageIds;

LOCALFUN void InitFilteredImages() {
	FilteredImages.push_back("libstdc++.so");
	FilteredImages.push_back("libm.so");
	FilteredImages.push_back("libc.so");
	FilteredImages.push_back("libgcc_s.so");
	FilteredImages.push_back("libselinux.so");
	FilteredImages.push_back("librt.so");
	FilteredImages.push_back("libdl.so");
	FilteredImages.push_back("libacl.so");
	FilteredImages.push_back("libattr.so");
	FilteredImages.push_back("ld-linux.so");
	FilteredImages.push_back("ld-linux-x86-64.so");

	FilteredImages.push_back("libpthread.so");
	FilteredImages.push_back("libpth.so");

	FilteredImages.push_back("libtbb.so");
	FilteredImages.push_back("libtbb_debug.so");
	FilteredImages.push_back("libtbbmalloc.so");
	FilteredImages.push_back("libtbbmalloc_debug.so");
	FilteredImages.push_back("libtbbmalloc_proxy.so");
	FilteredImages.push_back("libtbbmalloc_proxy_debug.so");
	FilteredImages.push_back("libtbb_preview.so");
	FilteredImages.push_back("libtbb_preview_debug.so");

	FilteredImages.push_back("libconcurrit.so");
	FilteredImages.push_back("libgflags.so");
	FilteredImages.push_back("libglog.so");

	FilteredImages.push_back("libbz2.so");
	FilteredImages.push_back("libnsl.so");
	FilteredImages.push_back("libssl.so");
	FilteredImages.push_back("libz.so");
	FilteredImages.push_back("libm.so");
}

/* ===================================================================== */

LOCALVAR volatile bool is_concurrit_loaded = false;

LOCALFUN VOID OnLoadConcurrit(IMG img) {
	safe_assert(!is_concurrit_loaded);

	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
	{
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
		{
			if(RTN_Name(rtn) == NativePinMonitorFunName) {
				RTN_Open(rtn);
				NativePinMonitorFunPtr = RTN_Funptr(rtn);
				RTN_Close(rtn);
				log_file << "Detected callback to concurrit: " << NativePinMonitorFunName << endl;

			} else if(RTN_Name(rtn) == NativePinEnableFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinEnable), IARG_FAST_ANALYSIS_CALL,
						IARG_END);

				RTN_Close(rtn);
				log_file << "Detected callback to concurrit: " << NativePinEnableFunName << endl;

			}  else if(RTN_Name(rtn) == NativePinDisableFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinDisable), IARG_FAST_ANALYSIS_CALL,
						IARG_END);

				RTN_Close(rtn);

				log_file << "Detected callback to concurrit: " << NativePinDisableFunName << endl;

			} else if(RTN_Name(rtn) == NativeThreadRestartFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(ThreadRestart), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);

				RTN_Close(rtn);

				log_file << "Detected callback to concurrit: " << NativeThreadRestartFunName << endl;

			} else if(RTN_Name(rtn) == NativeStartInstrumentFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(StartInstrument), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);

				RTN_Close(rtn);

				log_file << "Detected callback to concurrit: " << NativeStartInstrumentFunName << endl;

			} else if(RTN_Name(rtn) == NativeEndInstrumentFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(EndInstrument), IARG_FAST_ANALYSIS_CALL,
						IARG_THREAD_ID, IARG_END);

				RTN_Close(rtn);

				log_file << "Detected callback to concurrit: " << NativeEndInstrumentFunName << endl;
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

LOCALFUN BOOL IsImageFiltered(IMG img) {
	if(IMG_IsMainExecutable(img)) {
		return TRUE;
	}

	UINT32 img_id = IMG_Id(img);

	FilteredImageIdsType::const_accessor c_acc;
	if(FilteredImageIds.find(c_acc, img_id)) {
		BOOL value = c_acc->second;
		safe_assert(value == TRUE || value == FALSE);
		return value;
	}
	c_acc.release();

	string img_name = IMG_Name(img);
	for(std::vector<string>::iterator itr = FilteredImages.begin(); itr < FilteredImages.end(); ++itr) {
		if(img_name.find(*itr) != string::npos) {

			log_file << "--- IMG --- " << IMG_Name(img) << endl;
			FilteredImageIdsType::accessor acc;
			FilteredImageIds.insert(acc, img_id);
			acc->second = TRUE;
			acc.release();

			/* ================================= */
			if (!is_concurrit_loaded && *itr == "libconcurrit.so") {
				OnLoadConcurrit(img);
			}
			/* ================================= */

			return TRUE;
		}
	}

	log_file << "+++ IMG +++ " << IMG_Name(img) << endl;
	FilteredImageIdsType::accessor acc;
	FilteredImageIds.insert(acc, img_id);
	acc->second = FALSE;
	return FALSE;
}

INLINE LOCALFUN BOOL IsRoutineFiltered(RTN rtn, BOOL loading = FALSE) {
	if(RTN_Valid(rtn)) {
		return IsImageFiltered(SEC_Img(RTN_Sec(rtn)));
	}
	return TRUE;
}

INLINE LOCALFUN BOOL IsTraceFiltered(TRACE trace) {
	return IsRoutineFiltered(TRACE_Rtn(trace));
}

/* ===================================================================== */

VOID Routine(RTN rtn, VOID *v)
{
//	if(!pin_enabled) return;

	// filter out standard libraries
	// we treat this while loading, since Routine can be called before ImageLoad is called
	if(IsRoutineFiltered(rtn, TRUE)) {
		return;
	}

	log_file << "+++ RTN +++ " << RTN_Name(rtn) << endl;

	// check plt routine
	if(RTN_Name(rtn) == ".plt") {
		rtn_plt_addr = RTN_Address(rtn);
		return;
	}

	RTN_Open(rtn);

	PinSourceLocation* loc = PinSourceLocation::get(rtn);

	// if this is a routine to forward to concurrit, then record its address
	safe_assert(PTR2ADDRINT(loc->pointer()) == RTN_Address(rtn));
	InstParams::CheckAndRecordRoutine(rtn, PTR2ADDRINT(loc->pointer()));

	// check if start/end instrument
	if(RTN_Name(rtn) == NativeStartInstrumentFunName) {
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(StartInstrument), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);

	} else if(RTN_Name(rtn) == NativeEndInstrumentFunName) {
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(EndInstrument), IARG_FAST_ANALYSIS_CALL,
				IARG_THREAD_ID, IARG_END);
	} else {
		// standard instrumentation
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(FuncEnter), IARG_FAST_ANALYSIS_CALL,
			   IARG_CONTEXT,
			   IARG_THREAD_ID, IARG_PTR, loc,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_END);
	}

	RTN_Close(rtn);
}

/* ===================================================================== */

LOCALFUN VOID ImageLoad(IMG img, VOID *) {

	// filter out standard libraries
	// also updates filtered image ids
	if(IsImageFiltered(img)) {
		return;
	}

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

LOCALFUN VOID ImageUnload(IMG img, VOID *) {
	log_file << "Unloading image: " << IMG_Name(img) << endl;

	// delete filtering info about this image
	UINT32 img_id = IMG_Id(img);
	FilteredImageIds.erase(img_id);
}

/* ===================================================================== */

LOCALFUN INLINE
VOID CallTrace(TRACE trace, INS ins) {
//	if (INS_IsCall(ins) && INS_IsProcedureCall(ins) && !INS_IsSyscall(ins)) {
//		if (!INS_IsDirectBranchOrCall(ins)) {
//			// Indirect call
//			PinSourceLocation* loc = PinSourceLocation::get(TRACE_Rtn(trace), INS_Address(ins));
//
//			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncCall), IARG_FAST_ANALYSIS_CALL,
//					IARG_CONTEXT,
//					IARG_THREAD_ID, IARG_BOOL, FALSE, IARG_PTR, loc,
//					IARG_BRANCH_TARGET_ADDR,
//					IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_FUNCARG_CALLSITE_VALUE, 1, IARG_END);
//
//		} else if (INS_IsDirectBranchOrCall(ins)) {
//			// Direct call
//			PinSourceLocation* loc = PinSourceLocation::get(TRACE_Rtn(trace), INS_Address(ins));
//
//			ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
//
//			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncCall), IARG_FAST_ANALYSIS_CALL,
//					IARG_CONTEXT,
//					IARG_THREAD_ID, IARG_PTR, TRUE, IARG_PTR, loc,
//					IARG_ADDRINT, target,
//					IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_FUNCARG_CALLSITE_VALUE, 1, IARG_END);
//
//		}
//	} else
	if (INS_IsRet(ins) && !INS_IsSysret(ins)) {
		RTN rtn = TRACE_Rtn(trace);

#if defined(TARGET_LINUX) && defined(TARGET_IA32)
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_runtime_resolve") return;
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_debug_state") return;
#endif
		PinSourceLocation* loc = PinSourceLocation::get(rtn, INS_Address(ins));
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncReturn), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_PTR, loc, IARG_FUNCRET_EXITPOINT_VALUE, IARG_ADDRINT, RTN_Address(rtn), IARG_END);
	}
}

/* ===================================================================== */

LOCALFUN INLINE
VOID MemoryTrace(TRACE trace, INS ins) {
	if (INS_IsStackRead(ins) || INS_IsStackWrite(ins))
		return;

	bool is_access = INS_IsMemoryWrite(ins) || INS_HasMemoryRead2(ins) || INS_IsMemoryRead(ins);
	if(!is_access) return;
	bool has_fallthrough = INS_HasFallThrough(ins);
	bool is_branchorcall = INS_IsBranchOrCall(ins);
	if(!has_fallthrough && !is_branchorcall) return;

	PinSourceLocation* loc = PinSourceLocation::get(TRACE_Rtn(trace), INS_Address(ins));

	/* ==================== */

	if (INS_IsMemoryWrite(ins)) {
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(MemWrite), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_PTR, loc, IARG_END);
	}

	/* ==================== */

	if (INS_HasMemoryRead2(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemRead), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_PTR, loc, IARG_END);
	}

	/* ==================== */

	if (INS_IsMemoryRead(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemRead), IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_PTR, loc, IARG_END);
	}

	/* ======================================== */

	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(MemAccessBefore), IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);

	/* ==================== */

	if (has_fallthrough) {
		INS_InsertPredicatedCall(ins, IPOINT_AFTER, AFUNPTR(MemAccessAfter), IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);
	}
	if (is_branchorcall) {
		INS_InsertPredicatedCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(MemAccessAfter), IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);
	}
}

/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v) {

//	if(!pin_enabled) return;

	if (!filter.SelectTrace(trace))
		return;

	if (IsTraceFiltered(trace))
		return;

	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
	{
			for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
			{
				if (INS_IsOriginal(ins)) {

					MemoryTrace(trace, ins);

					CallTrace(trace, ins);
				}
			}
		}
	}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v) {
	timer.stop();

	printf("\nTIME for program: %s\n", timer.ElapsedTimeToString().c_str());

	PIN_DeleteThreadDataKey(tls_key);

	log_file.close();
}

/* ===================================================================== */

LOCALFUN void OnSig(THREADID threadIndex, CONTEXT_CHANGE_REASON reason,
		const CONTEXT *ctxtFrom, CONTEXT *ctxtTo, INT32 sig, VOID *v) {
	if (ctxtFrom != 0) {
		ADDRINT address = PIN_GetContextReg(ctxtFrom, REG_INST_PTR);
		cerr << "SIG signal=" << sig << " on thread " << threadIndex
				<< " at address " << hex << address << dec << " ";
	}

	switch (reason) {
	case CONTEXT_CHANGE_REASON_FATALSIGNAL:
		cerr << "FATALSIG" << sig;
		break;
	case CONTEXT_CHANGE_REASON_SIGNAL:
		cerr << "SIGNAL " << sig;
		break;
	case CONTEXT_CHANGE_REASON_SIGRETURN:
		cerr << "SIGRET";
		break;

	case CONTEXT_CHANGE_REASON_APC:
		cerr << "APC";
		break;

	case CONTEXT_CHANGE_REASON_EXCEPTION:
		cerr << "EXCEPTION";
		break;

	case CONTEXT_CHANGE_REASON_CALLBACK:
		cerr << "CALLBACK";
		break;

	default:
		break;
	}
	cerr << std::endl;
}

/* ===================================================================== */

LOCALFUN ThreadLocalState* GetThreadLocalState(THREADID tid) {
	safe_assert(tid == PIN_ThreadId());
	VOID* ptr = PIN_GetThreadData(tls_key, tid);
	safe_assert(ptr != NULL);
	ThreadLocalState* tls = static_cast<ThreadLocalState*>(ptr);
	safe_assert(tls->tid() == tid);
	return tls;
}

LOCALFUN VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags,
		VOID *v) {
	safe_assert(threadid == PIN_ThreadId());
	log_file << "Thread "<< threadid << " starting..." << endl;

	GLB_LOCK();
	PIN_SetThreadData(tls_key, new ThreadLocalState(threadid), threadid);
	GLB_UNLOCK();
}

// This routine is executed every time a thread is destroyed.
LOCALFUN VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code,
		VOID *v) {
	safe_assert(threadid == PIN_ThreadId());

	log_file << "Thread "<< threadid << " ending..." << endl;
}

LOCALFUN VOID ThreadLocalDestruct(VOID* ptr) {
	if (ptr != NULL) {
		ThreadLocalState* o = static_cast<ThreadLocalState*>(ptr);
		delete o;
	}
}

/* ===================================================================== */

std::vector<std::string> TokenizeStringToVector(char* str, const char* tokens) {
	std::vector<std::string> strlist;
	char * pch;
	pch = strtok (str, tokens);
	while (pch != NULL)
	{
		strlist.push_back(std::string(pch));
		pch = strtok (NULL, tokens);
	}
	return strlist;
}


int main(int argc, CHAR *argv[]) {
	PIN_InitSymbols();

	if (PIN_Init(argc, argv)) {
		return Usage();
	}

	// read function names to instrument
	InstParams::ParseFile(KnobFinFile.Value().c_str());

//	control.CheckKnobs(Handler, 0);
	skipper.CheckKnobs(0);

	log_file.open(KnobLogFile.Value().c_str());

	InitFilteredImages();

	tls_key = PIN_CreateThreadDataKey(ThreadLocalDestruct);
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	PIN_AddFiniFunction(Fini, 0);

	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);

	RTN_AddInstrumentFunction(Routine, 0);

	TRACE_AddInstrumentFunction(Trace, 0);
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
