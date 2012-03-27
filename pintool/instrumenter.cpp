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
#include "timer.h"
#include "sharedaccess.h"
#include "pinmonitor.h"
#include "tbb/concurrent_hash_map.h"

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<BOOL> KnobEarlyOut(KNOB_MODE_WRITEONCE, "pintool", "early_out", "0",
		"Exit after tracing the first region.");

/* ===================================================================== */

INT32 Usage() {
	cerr << "This pin tool does instrumentation for concurrit\n\n";

	cerr << KNOB_BASE::StringKnobSummary();

	cerr << endl;

	return -1;
}

/* ===================================================================== */

#define PTR2ADDRINT(p)	(reinterpret_cast<ADDRINT>(p))

/* ===================================================================== */

VOID ShowN(UINT32 n, VOID *ea) {
	// Print out the bytes in "big endian even though they are in memory little endian.
	// This is most natural for 8B and 16B quantities that show up most frequently.
	// The address pointed to
	cout << std::setfill('0');
	UINT8 b[512];
	UINT8* x;
	if (n > 512)
		x = new UINT8[n];
	else
		x = b;
	PIN_SafeCopy(x, static_cast<UINT8*>(ea), n);
	for (UINT32 i = 0; i < n; i++) {
		cout << std::setw(2) << static_cast<UINT32>(x[n - i - 1]);
		if (((reinterpret_cast<ADDRINT>(ea) + n - i - 1) & 0x3) == 0
				&& i < n - 1)
			cout << "_";
	}
	if (n > 512)
		delete[] x;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

LOCALVAR FILTER filter;
LOCALVAR concurrit::Timer timer("Pin running time");

//LOCALVAR CONTROL control;
LOCALVAR SKIPPER skipper;
LOCALVAR TLS_KEY tls_key;

LOCALVAR PIN_LOCK lock;
#define GLB_LOCK_INIT()		InitLock(&lock)
#define GLB_LOCK()			GetLock(&lock, 1)
#define GLB_UNLOCK()		ReleaseLock(&lock)

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
			ASSERTX(loc != NULL);
		} else {
			loc = new PinSourceLocation(rtn, address);
			addrToLoc_.insert(acc, address);
			acc->second = loc;
		}
		ASSERTX(loc != NULL);
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

VOID // PIN_FAST_ANALYSIS_CALL
FuncCall(const CONTEXT * ctxt, THREADID threadid, BOOL direct, PinSourceLocation* loc_src,
		ADDRINT target, ADDRINT arg0, ADDRINT arg1) {

	PinSourceLocation* loc_target = PinSourceLocation::get(target);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::FuncCall;
	info.threadid = threadid;
	info.addr = loc_target->pointer();
	info.direct = direct;
	info.loc_src = loc_src;
	info.loc_target = loc_target;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->FuncCall(threadid, loc_target->pointer(), direct, loc_src, loc_target);
}

/* ===================================================================== */

VOID // PIN_FAST_ANALYSIS_CALL
FuncEnter(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc,
		ADDRINT arg0, ADDRINT arg1) {

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::FuncEnter;
	info.threadid = threadid;
	info.addr = loc->pointer();
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->FuncEnter(threadid, loc->pointer(), loc);
}

VOID // PIN_FAST_ANALYSIS_CALL
FuncReturn(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc, ADDRINT ret0) {

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::FuncReturn;
	info.threadid = threadid;
	info.addr = loc->pointer();
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

VOID // PIN_FAST_ANALYSIS_CALL
MemAccessBefore(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc) {

//	CaptureAddrSize(threadid, addr, size);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemAccessBefore;
	info.threadid = threadid;
	info.loc_src = loc;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemWriteBefore(threadid, addr, size, loc);

}

VOID // PIN_FAST_ANALYSIS_CALL
MemAccessAfter(const CONTEXT * ctxt, THREADID threadid, PinSourceLocation* loc) {

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

VOID // PIN_FAST_ANALYSIS_CALL
MemWrite(const CONTEXT * ctxt, THREADID threadid, VOID * addr, UINT32 size) {

	CaptureAddrSize(threadid, addr, size);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemWrite;
	info.threadid = threadid;
	info.addr = addr;
	info.size = size;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemReadBefore(threadid, addr, size, loc);
}

VOID // PIN_FAST_ANALYSIS_CALL
MemRead(const CONTEXT * ctxt, THREADID threadid, VOID * addr, UINT32 size) {

	CaptureAddrSize(threadid, addr, size);

	concurrit::PinMonitorCallInfo info;
	info.type = concurrit::MemRead;
	info.threadid = threadid;
	info.addr = addr;
	info.size = size;

	CallNativePinMonitor(ctxt, threadid, &info);
	// monitor->MemReadBefore(threadid, addr, size, loc);
}

/* ===================================================================== */
static const char* NativePinEnableFunName = "EnablePinTool";
static const char* NativePinDisableFunName = "DisablePinTool";

const UINT32 ENABLED  = 1;
const UINT32 DISABLED = 0;

LOCALVAR volatile UINT32 pin_status = ENABLED;

VOID PIN_FAST_ANALYSIS_CALL
PinEnableDisable(const CONTEXT * ctxt, UINT32 command) {
	if(pin_status == command) {
		return;
	}

	do {
		pin_status = command;
	} while(FALSE);

	PIN_RemoveInstrumentation();
}

/* ===================================================================== */

//LOCALFUN VOID Fini(INT32 code, VOID *v);
//
//LOCALFUN VOID Handler(CONTROL_EVENT ev, VOID *, CONTEXT * ctxt, VOID *,
//		THREADID) {
//	switch (ev) {
//	case CONTROL_START:
//		enabled = TRUE;
//		PIN_RemoveInstrumentation();
//#if defined(TARGET_IA32) || defined(TARGET_IA32E)
//		// So that the rest of the current trace is re-instrumented.
//		if (ctxt) PIN_ExecuteAt (ctxt);
//#endif
//		break;
//
//	case CONTROL_STOP:
//		enabled = FALSE;
//		PIN_RemoveInstrumentation();
//		if (KnobEarlyOut) {
//			cerr << "Exiting due to -early_out" << endl;
//			Fini(0, NULL);
//			exit(0);
//		}
//#if defined(TARGET_IA32) || defined(TARGET_IA32E)
//		// So that the rest of the current trace is re-instrumented.
//		if (ctxt) PIN_ExecuteAt (ctxt);
//#endif
//		break;
//
//	default:
//		ASSERTX(FALSE);
//	}
//}

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
	FilteredImages.push_back("libpthread.so");
	FilteredImages.push_back("ld-linux.so");
	FilteredImages.push_back("ld-linux-x86-64.so");

	FilteredImages.push_back("libconcurrit.so");
	FilteredImages.push_back("libgflags.so");
	FilteredImages.push_back("libglog.so");
}

/* ===================================================================== */

LOCALVAR volatile bool is_concurrit_loaded = false;

LOCALFUN VOID OnLoadConcurrit(IMG img) {
	ASSERTX(!is_concurrit_loaded);

	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
	{
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
		{
			if(RTN_Name(rtn) == NativePinMonitorFunName) {
				RTN_Open(rtn);
				NativePinMonitorFunPtr = RTN_Funptr(rtn);
				RTN_Close(rtn);
				cout << "Detected callback " << NativePinMonitorFunName << endl;

			} else if(RTN_Name(rtn) == NativePinEnableFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinEnableDisable), IARG_FAST_ANALYSIS_CALL,
						   IARG_CONTEXT,
						   IARG_UINT32, ENABLED, IARG_END);

				RTN_Close(rtn);
				cout << "Detected callback " << NativePinEnableFunName << endl;

			}  else if(RTN_Name(rtn) == NativePinDisableFunName) {
				RTN_Open(rtn);

				RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(PinEnableDisable), IARG_FAST_ANALYSIS_CALL,
						   IARG_CONTEXT,
						   IARG_UINT32, DISABLED, IARG_END);

				RTN_Close(rtn);

				cout << "Detected callback " << NativePinDisableFunName << endl;
			}
		}
	}
	ASSERTX(NativePinMonitorFunPtr != NULL);

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

			/* ================================= */
			if (!is_concurrit_loaded && *itr == "libconcurrit.so") {
				OnLoadConcurrit(img);
			}
			/* ================================= */

			cout << "--- IMG --- " << IMG_Name(img) << endl;
			FilteredImageIdsType::accessor acc;
			FilteredImageIds.insert(acc, img_id);
			acc->second = TRUE;
			return TRUE;
		}
	}

	cout << "+++ IMG +++ " << IMG_Name(img) << endl;
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
	if(pin_status == DISABLED) return;

	// filter out standard libraries
	// we treat this while loading, since Routine can be called before ImageLoad is called
	if(IsRoutineFiltered(rtn, TRUE)) {
		return;
	}

	cout << "+++ RTN +++ " << RTN_Name(rtn) << endl;

	RTN_Open(rtn);

	PinSourceLocation* loc = PinSourceLocation::get(rtn);

	RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(FuncEnter), // IARG_FAST_ANALYSIS_CALL,
			   IARG_CONTEXT,
			   IARG_THREAD_ID, IARG_PTR, loc,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			   IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			   IARG_END);

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
//			cout << "\t+++ RTN +++ " << RTN_Name(rtn) << endl;
//
//			RTN_Open(rtn);
//
//			PinSourceLocation* loc = PinSourceLocation::get(rtn);
//
//			RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(FuncEnter), // IARG_FAST_ANALYSIS_CALL,
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
	cout << "Unloading image: " << IMG_Name(img) << endl;

	// delete filtering info about this image
	UINT32 img_id = IMG_Id(img);
	FilteredImageIds.erase(img_id);
}

/* ===================================================================== */

VOID CallTrace(TRACE trace, INS ins) {
	if (INS_IsCall(ins) && INS_IsProcedureCall(ins) && !INS_IsSyscall(ins)) {
		if (!INS_IsDirectBranchOrCall(ins)) {
			// Indirect call
			PinSourceLocation* loc = PinSourceLocation::get(TRACE_Rtn(trace), INS_Address(ins));

			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncCall), // IARG_FAST_ANALYSIS_CALL,
					IARG_CONTEXT,
					IARG_THREAD_ID, IARG_BOOL, FALSE, IARG_PTR, loc,
					IARG_BRANCH_TARGET_ADDR,
					IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_FUNCARG_CALLSITE_VALUE, 1, IARG_END);

		} else if (INS_IsDirectBranchOrCall(ins)) {
			// Direct call
			PinSourceLocation* loc = PinSourceLocation::get(TRACE_Rtn(trace), INS_Address(ins));

			ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);

			INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncCall), // IARG_FAST_ANALYSIS_CALL,
					IARG_CONTEXT,
					IARG_THREAD_ID, IARG_PTR, TRUE, IARG_PTR, loc,
					IARG_ADDRINT, target,
					IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_FUNCARG_CALLSITE_VALUE, 1, IARG_END);

		}
	} else if (INS_IsRet(ins) && !INS_IsSysret(ins)) {
		RTN rtn = TRACE_Rtn(trace);

#if defined(TARGET_LINUX) && defined(TARGET_IA32)
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_runtime_resolve") return;
		if( RTN_Valid(rtn) && RTN_Name(rtn) == "_dl_debug_state") return;
#endif
		PinSourceLocation* loc = PinSourceLocation::get(rtn, INS_Address(ins));
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(FuncReturn), // IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_PTR, loc, IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);
	}
}

/* ===================================================================== */

VOID MemoryTrace(TRACE trace, INS ins) {
	if (INS_IsStackRead(ins) || INS_IsStackWrite(ins))
		return;

	bool is_access = INS_IsMemoryWrite(ins) || INS_HasMemoryRead2(ins) || INS_IsMemoryRead(ins);
	if(!is_access) return;
	bool has_fallthrough = INS_HasFallThrough(ins);
	bool is_branchorcall = INS_IsBranchOrCall(ins);
	if(!has_fallthrough && !is_branchorcall) return;

	/* ==================== */

	PinSourceLocation* loc = PinSourceLocation::get(TRACE_Rtn(trace), INS_Address(ins));
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(MemAccessBefore), // IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);

	/* ==================== */

	if (INS_IsMemoryWrite(ins)) {
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(MemWrite), // IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
	}

	/* ==================== */

	if (INS_HasMemoryRead2(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemRead), // IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_END);
	}

	/* ==================== */

	if (INS_IsMemoryRead(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(MemRead), // IARG_FAST_ANALYSIS_CALL,
				IARG_CONTEXT,
				IARG_THREAD_ID, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
	}

	/* ==================== */

	if (has_fallthrough) {
		INS_InsertPredicatedCall(ins, IPOINT_AFTER, AFUNPTR(MemAccessAfter), // IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);
	}
	if (is_branchorcall) {
		INS_InsertPredicatedCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(MemAccessAfter), // IARG_FAST_ANALYSIS_CALL,
			IARG_CONTEXT,
			IARG_THREAD_ID, IARG_PTR, loc, IARG_END);
	}
}

/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v) {
	if(pin_status == DISABLED) return;

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
}

/* ===================================================================== */

LOCALFUN void OnSig(THREADID threadIndex, CONTEXT_CHANGE_REASON reason,
		const CONTEXT *ctxtFrom, CONTEXT *ctxtTo, INT32 sig, VOID *v) {
	if (ctxtFrom != 0) {
		ADDRINT address = PIN_GetContextReg(ctxtFrom, REG_INST_PTR);
		cout << "SIG signal=" << sig << " on thread " << threadIndex
				<< " at address " << hex << address << dec << " ";
	}

	switch (reason) {
	case CONTEXT_CHANGE_REASON_FATALSIGNAL:
		cout << "FATALSIG" << sig;
		break;
	case CONTEXT_CHANGE_REASON_SIGNAL:
		cout << "SIGNAL " << sig;
		break;
	case CONTEXT_CHANGE_REASON_SIGRETURN:
		cout << "SIGRET";
		break;

	case CONTEXT_CHANGE_REASON_APC:
		cout << "APC";
		break;

	case CONTEXT_CHANGE_REASON_EXCEPTION:
		cout << "EXCEPTION";
		break;

	case CONTEXT_CHANGE_REASON_CALLBACK:
		cout << "CALLBACK";
		break;

	default:
		break;
	}
	cout << std::endl;
}

/* ===================================================================== */

class ThreadLocalState {
public:
	ThreadLocalState(THREADID tid) {
		printf("Creating thread local state for tid %d...\n", tid);

		ASSERTX(PIN_IsApplicationThread());
		ASSERTX(tid != INVALID_THREADID);
		ASSERTX(tid == PIN_ThreadId());

		tid_ = tid;
	}

	~ThreadLocalState() {
		printf("Deleting thread local state for tid %d...\n", tid_);
	}

private:
	DECL_FIELD(THREADID, tid)
};

/* ===================================================================== */

LOCALFUN ThreadLocalState* GetThreadLocalState(THREADID tid) {
	ASSERTX(tid == PIN_ThreadId());
	VOID* ptr = PIN_GetThreadData(tls_key, tid);
	ASSERTX(ptr != NULL);
	ThreadLocalState* tls = static_cast<ThreadLocalState*>(ptr);
	ASSERTX(tls->tid() == tid);
	return tls;
}

LOCALFUN VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags,
		VOID *v) {
	ASSERTX(threadid == PIN_ThreadId());
	printf("Thread %d starting...\n", threadid);

	GLB_LOCK();
	PIN_SetThreadData(tls_key, new ThreadLocalState(threadid), threadid);
	GLB_UNLOCK();
}

// This routine is executed every time a thread is destroyed.
LOCALFUN VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code,
		VOID *v) {
	ASSERTX(threadid == PIN_ThreadId());

	printf("Thread %d ending...\n", threadid);
}

LOCALFUN VOID ThreadLocalDestruct(VOID* ptr) {
	if (ptr != NULL) {
		ThreadLocalState* o = static_cast<ThreadLocalState*>(ptr);
		delete o;
	}
}

/* ===================================================================== */

int main(int argc, CHAR *argv[]) {
	PIN_InitSymbols();

	if (PIN_Init(argc, argv)) {
		return Usage();
	}

//	control.CheckKnobs(Handler, 0);
	skipper.CheckKnobs(0);

	InitFilteredImages();

	tls_key = PIN_CreateThreadDataKey(ThreadLocalDestruct);
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	PIN_AddFiniFunction(Fini, 0);

	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);

	RTN_AddInstrumentFunction(Routine, 0);

	TRACE_AddInstrumentFunction(Trace, 0);
	PIN_AddContextChangeFunction(OnSig, 0);

	filter.Activate();

	// GLB_LOCK_INIT(); // uncomment if needed

	// start the timer
	timer.start();

	// Never returns

	PIN_StartProgram();

	return 0;
}

/* ===================================================================== */
