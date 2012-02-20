/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2011 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

/* ===================================================================== */
/*
  @ORIGINAL_AUTHOR: Robert Cohn
*/

/* ===================================================================== */
/*! @file
 *  This file contains a tool that generates instructions traces with values.
 *  It is designed to help debugging.
 */



#include "pin.H"
#include "instlib.H"
#include "portability.H"
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace INSTLIB;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,         "pintool",
    "o", "debugtrace.out", "trace file");
KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE,                "pintool",
    "i", "0", "append pid to output");
KNOB<THREADID>   KnobWatchThread(KNOB_MODE_WRITEONCE,                "pintool",
    "watch_thread", "-1", "thread to watch, -1 for all");
KNOB<BOOL>   KnobFlush(KNOB_MODE_WRITEONCE,                "pintool",
    "flush", "0", "Flush output after every instruction");
KNOB<BOOL>   KnobSymbols(KNOB_MODE_WRITEONCE,       "pintool",
    "symbols", "1", "Include symbol information");
KNOB<BOOL>   KnobLines(KNOB_MODE_WRITEONCE,       "pintool",
    "lines", "0", "Include line number information");
KNOB<BOOL>   KnobTraceInstructions(KNOB_MODE_WRITEONCE,       "pintool",
    "instruction", "0", "Trace instructions");
KNOB<BOOL>   KnobTraceCalls(KNOB_MODE_WRITEONCE,       "pintool",
    "call", "1", "Trace calls");
KNOB<BOOL>   KnobTraceMemory(KNOB_MODE_WRITEONCE,       "pintool",
    "memory", "0", "Trace memory");
KNOB<BOOL>   KnobSilent(KNOB_MODE_WRITEONCE,       "pintool",
    "silent", "0", "Do everything but write file (for debugging).");
KNOB<BOOL> KnobEarlyOut(KNOB_MODE_WRITEONCE, "pintool", "early_out", "0" , "Exit after tracing the first region.");


/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool does instrumentation for concurrit\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

/* ===================================================================== */

#define DECL_FIELD(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type& name() { return name##_; } \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\

/* ===================================================================== */

// this is from GNU C library manual

/* Subtract the `struct timeval' values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0.  */

int timeval_subtract(struct timeval *result, struct timeval *_x, struct timeval *_y) {
	struct timeval x = *_x;
	struct timeval y = *_y;
	/* Perform the carry for the later subtraction by updating y. */
	if (x.tv_usec < y.tv_usec) {
		int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
		y.tv_usec -= 1000000 * nsec;
		y.tv_sec += nsec;
	}
	if (x.tv_usec - y.tv_usec > 1000000) {
		int nsec = (y.tv_usec - x.tv_usec) / 1000000;
		y.tv_usec += 1000000 * nsec;
		y.tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 tv_usec is certainly positive. */
	result->tv_sec = x.tv_sec - y.tv_sec;
	result->tv_usec = x.tv_usec - y.tv_usec;

	/* Return 1 if result is negative. */
	return x.tv_sec < y.tv_sec;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

typedef UINT64 COUNTER;

LOCALVAR INT32 enabled = 0;

LOCALVAR FILTER filter;

LOCALVAR ICOUNT icount;

LOCALVAR struct timeval startTime;
LOCALVAR struct timeval endTime;
LOCALVAR struct timeval runningTime;

LOCALVAR PIN_LOCK lock;
#define GLB_LOCK_INIT()		InitLock(&lock)
#define GLB_LOCK()			GetLock(&lock, 1)
#define GLB_UNLOCK()		ReleaseLock(&lock)

/* ===================================================================== */

LOCALFUN BOOL Emit(THREADID threadid)
{
    if (!enabled || 
        KnobSilent || 
        (KnobWatchThread != static_cast<THREADID>(-1) && KnobWatchThread != threadid))
        return false;
    return true;
}

LOCALFUN VOID Flush()
{
    if (KnobFlush)
        count << flush;
}

/* ===================================================================== */

LOCALFUN VOID Fini(INT32 code, VOID *v)

LOCALFUN VOID Handler(CONTROL_EVENT ev, VOID *, CONTEXT * ctxt, VOID *, THREADID)
{
    switch(ev)
    {
      case CONTROL_START:
        enabled = 1;
        PIN_RemoveInstrumentation();
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
    // So that the rest of the current trace is re-instrumented.
    if (ctxt) PIN_ExecuteAt (ctxt);
#endif   
        break;

      case CONTROL_STOP:
        enabled = 0;
        PIN_RemoveInstrumentation();
        if (KnobEarlyOut)
        {
            cerr << "Exiting due to -early_out" << endl;
            Fini(0, NULL);
            exit(0);
        }
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
    // So that the rest of the current trace is re-instrumented.
    if (ctxt) PIN_ExecuteAt (ctxt);
#endif   
        break;

      default:
        ASSERTX(false);
    }
}


/* ===================================================================== */

VOID EmitNoValues(THREADID threadid, string * str)
{
    if (!Emit(threadid))
        return;
    
    out
        << *str
        << endl;

    Flush();
}

VOID Emit1Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val)
{
    if (!Emit(threadid))
        return;
    
    out
        << *str << " | "
        << *reg1str << " = " << reg1val
        << endl;

    Flush();
}

VOID Emit2Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val)
{
    if (!Emit(threadid))
        return;
    
    out
        << *str << " | "
        << *reg1str << " = " << reg1val
        << ", " << *reg2str << " = " << reg2val
        << endl;
    
    Flush();
}

VOID Emit3Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val, string * reg3str, ADDRINT reg3val)
{
    if (!Emit(threadid))
        return;
    
    out
        << *str << " | "
        << *reg1str << " = " << reg1val
        << ", " << *reg2str << " = " << reg2val
        << ", " << *reg3str << " = " << reg3val
        << endl;
    
    Flush();
}


VOID Emit4Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val, string * reg3str, ADDRINT reg3val, string * reg4str, ADDRINT reg4val)
{
    if (!Emit(threadid))
        return;
    
    out
        << *str << " | "
        << *reg1str << " = " << reg1val
        << ", " << *reg2str << " = " << reg2val
        << ", " << *reg3str << " = " << reg3val
        << ", " << *reg4str << " = " << reg4val
        << endl;
    
    Flush();
}


const UINT32 MaxEmitArgs = 4;

AFUNPTR emitFuns[] = 
{
    AFUNPTR(EmitNoValues), AFUNPTR(Emit1Values), AFUNPTR(Emit2Values), AFUNPTR(Emit3Values), AFUNPTR(Emit4Values)
};

/* ===================================================================== */
#if !defined(TARGET_IPF)

VOID EmitXMM(THREADID threadid, UINT32 regno, PIN_REGISTER* xmm)
{
    if (!Emit(threadid))
        return;
    count << "\t\t\tXMM" << dec << regno << " := " << setfill('0') << hex;
    out.unsetf(ios::showbase);
    for(int i=0;i<16;i++) {
        if (i==4 || i==8 || i==12)
            count << "_";
        count << setw(2) << (int)xmm->byte[15-i]; // msb on the left as in registers
    }
    out  << setfill(' ') << endl;
    out.setf(ios::showbase);
    Flush();
}

VOID AddXMMEmit(INS ins, IPOINT point, REG xmm_dst) 
{
    INS_InsertCall(ins, point, AFUNPTR(EmitXMM), IARG_THREAD_ID,
                   IARG_UINT32, xmm_dst - REG_XMM0,
                   IARG_REG_CONST_REFERENCE, xmm_dst,
                   IARG_END);
}
#endif

VOID AddEmit(INS ins, IPOINT point, string & traceString, UINT32 regCount, REG regs[])
{
    if (regCount > MaxEmitArgs)
        regCount = MaxEmitArgs;
    
    IARGLIST args = IARGLIST_Alloc();
    for (UINT32 i = 0; i < regCount; i++)
    {
        IARGLIST_AddArguments(args, IARG_PTR, new string(REG_StringShort(regs[i])), IARG_REG_VALUE, regs[i], IARG_END);
    }

    INS_InsertCall(ins, point, emitFuns[regCount], IARG_THREAD_ID,
                   IARG_PTR, new string(traceString),
                   IARG_IARGLIST, args,
                   IARG_END);
    IARGLIST_Free(args);
}

LOCALVAR VOID *WriteEa[PIN_MAX_THREADS];

VOID CaptureWriteEa(THREADID threadid, VOID * addr)
{
    WriteEa[threadid] = addr;
}

VOID ShowN(UINT32 n, VOID *ea)
{
    out.unsetf(ios::showbase);
    // Print out the bytes in "big endian even though they are in memory little endian.
    // This is most natural for 8B and 16B quantities that show up most frequently.
    // The address pointed to 
    count << std::setfill('0');
    UINT8 b[512];
    UINT8* x;
    if (n > 512)
        x = new UINT8[n];
    else
        x = b;
    PIN_SafeCopy(x,static_cast<UINT8*>(ea),n);    
    for (UINT32 i = 0; i < n; i++)
    {
        count << std::setw(2) <<  static_cast<UINT32>(x[n-i-1]);
        if (((reinterpret_cast<ADDRINT>(ea)+n-i-1)&0x3)==0 && i<n-1)
            count << "_";
    }
    count << std::setfill(' ');
    out.setf(ios::showbase);
    if (n>512)
        delete [] x;
}


VOID EmitWrite(THREADID threadid, UINT32 size)
{
    if (!Emit(threadid))
        return;
    
    count << "                                 Write ";
    
    VOID * ea = WriteEa[threadid];
    
    switch(size)
    {
      case 0:
        count << "0 repeat count" << endl;
        break;
        
      case 1:
        {
            UINT8 x;
            PIN_SafeCopy(&x, static_cast<UINT8*>(ea), 1);
            count << "*(UINT8*)" << ea << " = " << static_cast<UINT32>(x) << endl;
        }
        break;
        
      case 2:
        {
            UINT16 x;
            PIN_SafeCopy(&x, static_cast<UINT16*>(ea), 2);
            count << "*(UINT16*)" << ea << " = " << x << endl;
        }
        break;
        
      case 4:
        {
            UINT32 x;
            PIN_SafeCopy(&x, static_cast<UINT32*>(ea), 4);
            count << "*(UINT32*)" << ea << " = " << x << endl;
        }
        break;
        
      case 8:
        {
            UINT64 x;
            PIN_SafeCopy(&x, static_cast<UINT64*>(ea), 8);
            count << "*(UINT64*)" << ea << " = " << x << endl;
        }
        break;
        
      default:
        count << "*(UINT" << dec << size * 8 << hex << ")" << ea << " = ";
        ShowN(size,ea);
        count << endl;
        break;
    }

    Flush();
}

VOID EmitRead(THREADID threadid, VOID * ea, UINT32 size)
{
    if (!Emit(threadid))
        return;
    
    count << "                                 Read ";

    switch(size)
    {
      case 0:
        count << "0 repeat count" << endl;
        break;
        
      case 1:
        {
            UINT8 x;
            PIN_SafeCopy(&x,static_cast<UINT8*>(ea),1);
            count << static_cast<UINT32>(x) << " = *(UINT8*)" << ea << endl;
        }
        break;
        
      case 2:
        {
            UINT16 x;
            PIN_SafeCopy(&x,static_cast<UINT16*>(ea),2);
            count << x << " = *(UINT16*)" << ea << endl;
        }
        break;
        
      case 4:
        {
            UINT32 x;
            PIN_SafeCopy(&x,static_cast<UINT32*>(ea),4);
            count << x << " = *(UINT32*)" << ea << endl;
        }
        break;
        
      case 8:
        {
            UINT64 x;
            PIN_SafeCopy(&x,static_cast<UINT64*>(ea),8);
            count << x << " = *(UINT64*)" << ea << endl;
        }
        break;
        
      default:
        ShowN(size,ea);
        count << " = *(UINT" << dec << size * 8 << hex << ")" << ea << endl;
        break;
    }

    Flush();
}


LOCALVAR INT32 indent = 0;

VOID Indent()
{
    for (INT32 i = 0; i < indent; i++)
    {
        count << "| ";
    }
}

VOID EmitICount()
{
    count << setw(10) << dec << icount.Count() << hex << " ";
}

VOID EmitDirectCall(THREADID threadid, string * str, INT32 tailCall, ADDRINT arg0, ADDRINT arg1)
{
    if (!Emit(threadid))
        return;
    
    EmitICount();

    if (tailCall)
    {
        // A tail call is like an implicit return followed by an immediate call
        indent--;
    }
    
    Indent();
    count << *str << "(" << arg0 << ", " << arg1 << ", ...)" << endl;

    indent++;

    Flush();
}

string FormatAddress(ADDRINT address, RTN rtn)
{
    string s = StringFromAddrint(address);
    
    if (KnobSymbols && RTN_Valid(rtn))
    {
        s += " " + IMG_Name(SEC_Img(RTN_Sec(rtn))) + ":";
        s += RTN_Name(rtn);

        ADDRINT delta = address - RTN_Address(rtn);
        if (delta != 0)
        {
            s += "+" + hexstr(delta, 4);
        }
    }

    if (KnobLines)
    {
        INT32 line;
        string file;
        
        PIN_GetSourceLocation(address, NULL, &line, &file);

        if (file != "")
        {
            s += " (" + file + ":" + decstr(line) + ")";
        }
    }
    return s;
}

VOID EmitIndirectCall(THREADID threadid, string * str, ADDRINT target, ADDRINT arg0, ADDRINT arg1)
{
    if (!Emit(threadid))
        return;
    
    EmitICount();
    Indent();
    count << *str;

    PIN_LockClient();
    
    string s = FormatAddress(target, RTN_FindByAddress(target));
    
    PIN_UnlockClient();
    
    count << s << "(" << arg0 << ", " << arg1 << ", ...)" << endl;
    indent++;

    Flush();
}

VOID EmitReturn(THREADID threadid, string * str, ADDRINT ret0)
{
    if (!Emit(threadid))
        return;
    
    EmitICount();
    indent--;
    if (indent < 0)
    {
        count << "@@@ return underflow\n";
        indent = 0;
    }
    
    Indent();
    count << *str << " returns: " << ret0 << endl;

    Flush();
}

        
VOID CallTrace(TRACE trace, INS ins)
{
    if (!KnobTraceCalls)
        return;

    if (INS_IsCall(ins) && !INS_IsDirectBranchOrCall(ins))
    {
        // Indirect call
        string s = "Call " + FormatAddress(INS_Address(ins), TRACE_Rtn(trace));
        s += " -> ";

        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(EmitIndirectCall), IARG_THREAD_ID,
                       IARG_PTR, new string(s), IARG_BRANCH_TARGET_ADDR,
                       IARG_G_ARG0_CALLER, IARG_G_ARG1_CALLER, IARG_END);
    }
    else if (INS_IsDirectBranchOrCall(ins))
    {
        // Is this a tail call?
        RTN sourceRtn = TRACE_Rtn(trace);
        RTN destRtn = RTN_FindByAddress(INS_DirectBranchOrCallTargetAddress(ins));

        if (INS_IsCall(ins)         // conventional call
            || sourceRtn != destRtn // tail call
        )
        {
            BOOL tailcall = !INS_IsCall(ins);
            
            string s = "";
            if (tailcall)
            {
                s += "Tailcall ";
            }
            else
            {
                if( INS_IsProcedureCall(ins) )
                    s += "Call ";
                else
                {
                    s += "PcMaterialization ";
                    tailcall=1;
                }
                
            }

            //s += INS_Mnemonic(ins) + " ";
            
            s += FormatAddress(INS_Address(ins), TRACE_Rtn(trace));
            s += " -> ";

            ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
        
            s += FormatAddress(target, RTN_FindByAddress(target));

            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(EmitDirectCall),
                           IARG_THREAD_ID, IARG_PTR, new string(s), IARG_BOOL, tailcall,
                           IARG_G_ARG0_CALLER, IARG_G_ARG1_CALLER, IARG_END);
        }
    }
    else if (INS_IsRet(ins))
    {
        RTN rtn =  TRACE_Rtn(trace);
        
#if defined(TARGET_LINUX) && defined(TARGET_IA32)
//        if( RTN_Name(rtn) ==  "_dl_debug_state") return;
        if( RTN_Valid(rtn) && RTN_Name(rtn) ==  "_dl_runtime_resolve") return;
#endif
        string tracestring = "Return " + FormatAddress(INS_Address(ins), rtn);
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(EmitReturn),
                       IARG_THREAD_ID, IARG_PTR, new string(tracestring), IARG_G_RESULT0, IARG_END);
    }
}
        

/* ===================================================================== */

VOID MemoryTrace(INS ins)
{
    if (!KnobTraceMemory)
        return;
    
    if(INS_IsStackRead(ins) && !INS_IsStackWrite(ins))
    	return;

    if (INS_IsMemoryWrite(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CaptureWriteEa), IARG_THREAD_ID, IARG_MEMORYWRITE_EA, IARG_END);

        if (INS_HasFallThrough(ins))
        {
            INS_InsertPredicatedCall(ins, IPOINT_AFTER, AFUNPTR(EmitWrite), IARG_THREAD_ID, IARG_MEMORYWRITE_SIZE, IARG_END);
        }
        if (INS_IsBranchOrCall(ins))
        {
            INS_InsertPredicatedCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(EmitWrite), IARG_THREAD_ID, IARG_MEMORYWRITE_SIZE, IARG_END);
        }
    }

    if (INS_HasMemoryRead2(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(EmitRead), IARG_THREAD_ID, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_END);
    }

    if (INS_IsMemoryRead(ins) && !INS_IsPrefetch(ins))
    {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(EmitRead), IARG_THREAD_ID, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
    }
}


/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v)
{
    if (!filter.SelectTrace(trace))
        return;
    
    if (enabled)
    {
        for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
        {
            for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
            {
            	if (INS_IsOriginal(ins)) {

					MemoryTrace(ins);

					CallTrace(trace, ins);
            	}
            }
        }
    }
}


/* ===================================================================== */

VOID Fini(INT32 code, VOID *v) {
	if(gettimeofday(&endTime, NULL)) {
		printf("Failed to get the end time!");
		exit(-1);
	}

	timeval_subtract(&runningTime, &endTime, &startTime);

	printf("\nTIME for program:\t%lu seconds, %lu microseconds\n", runningTime.tv_sec, runningTime.tv_usec);

	PIN_DeleteThreadDataKey(tls_key);
}


    
/* ===================================================================== */

LOCALFUN void OnSig(THREADID threadIndex,
                  CONTEXT_CHANGE_REASON reason, 
                  const CONTEXT *ctxtFrom,
                  CONTEXT *ctxtTo,
                  INT32 sig, 
                  VOID *v)
{
    if (ctxtFrom != 0)
    {
        ADDRINT address = PIN_GetContextReg(ctxtFrom, REG_INST_PTR);
        count << "SIG signal=" << sig << " on thread " << threadIndex
            << " at address " << hex << address << dec << " ";
    }

    switch (reason)
    {
      case CONTEXT_CHANGE_REASON_FATALSIGNAL:
        count << "FATALSIG" << sig;
        break;
      case CONTEXT_CHANGE_REASON_SIGNAL:
        count << "SIGNAL " << sig;
        break;
      case CONTEXT_CHANGE_REASON_SIGRETURN:
        count << "SIGRET";
        break;
   
      case CONTEXT_CHANGE_REASON_APC:
        count << "APC";
        break;

      case CONTEXT_CHANGE_REASON_EXCEPTION:
        count << "EXCEPTION";
        break;

      case CONTEXT_CHANGE_REASON_CALLBACK:
        count << "CALLBACK";
        break;

      default: 
        break;
    }
    count << std::endl;
}

/* ===================================================================== */

LOCALVAR CONTROL control;
LOCALVAR SKIPPER skipper;

LOCALVAR TLS_KEY tls_key;

/* ===================================================================== */

class ThreadLocalState {
public:
	ThreadLocalState(THREADID tid) {
		printf("Creating thread local state for tid %d...\n", tid);

		assert(PIN_IsApplicationThread());
		assert(tid != INVALID_THREADID);
		assert(tid == PIN_ThreadId());

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
	assert(tid == PIN_ThreadId());
	VOID* ptr = PIN_GetThreadData(tls_key, tid);
	assert(ptr != NULL);
	ThreadLocalState* tls = static_cast<ThreadLocalState*>(ptr);
	assert(tls->tid() == tid);
	return tls;
}

LOCALFUN VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    GLB_LOCK();
    assert(threadid == PIN_ThreadId());
    printf("Thread %d starting...\n", threadid);
    PIN_SetThreadData(tls_key, new ThreadLocalState(threadid), threadid);
    GLB_UNLOCK();
}

// This routine is executed every time a thread is destroyed.
LOCALFUN VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	assert(threadid == PIN_ThreadId());

	printf("Thread %d ending...\n", threadid);
}

LOCALFUN VOID ThreadLocalDestruct(VOID* ptr) {
	if(ptr != NULL) {
		ThreadLocalState* o = static_cast<ThreadLocalState*>(ptr);
		delete o;
	}
}

/* ===================================================================== */

LOCALVAR bool pthreadLoaded = false;
LOCALVAR UINT32 pthreadIMGID = 0;

LOCALFUN VOID ImageLoad(IMG img, VOID *) {

	RTN rtn = RTN_FindByName(img, "_ZN6counit11BeginStrandEPKc");

	if (RTN_Valid(rtn)) {
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(StrandStart),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
		RTN_Close(rtn);
	}

	/******************************************************/

	if(pthreadLoaded) return;

	rtn = RTN_FindByName(img, "initSection");

	if (RTN_Valid(rtn)) {

		//--------------------------
		printf("Loading libpthread...\n");
		pthreadLoaded = true;
		pthreadIMGID = IMG_Id(img);
		//--------------------------

		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_AFTER, AFUNPTR(LockAcquire),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
		RTN_Close(rtn);
	}

	/******************************************************/

	rtn = RTN_FindByName(img, "commitSection");

	if (RTN_Valid(rtn)) {
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(LockRelease),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
		RTN_Close(rtn);
	}

	/******************************************************/

	rtn = RTN_FindByName(img, "pthread_rwlock_rdlock");

	if (RTN_Valid(rtn)) {
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(RLockAcquire),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
		RTN_Close(rtn);
	}

	/******************************************************/

	rtn = RTN_FindByName(img, "pthread_rwlock_wrlock");

	if (RTN_Valid(rtn)) {
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(WLockAcquire),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
		RTN_Close(rtn);
	}

	/******************************************************/

	rtn = RTN_FindByName(img, "pthread_rwlock_unlock");

	if (RTN_Valid(rtn)) {
		RTN_Open(rtn);
		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(RWLockRelease),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);
		RTN_Close(rtn);
	}

	/******************************************************/

	rtn = RTN_FindByName(img, "__pthread_create_2_1");

	if (RTN_Valid(rtn)) {

		RTN_Open(rtn);

		RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(ThreadCreate),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);

		RTN_Close(rtn);
	}

	/******************************************************/

	rtn = RTN_FindByName(img, "pthread_join");

	if (RTN_Valid(rtn)) {

		RTN_Open(rtn);

		RTN_InsertCall(rtn, IPOINT_AFTER, AFUNPTR(ThreadJoin),
				IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_INST_PTR, IARG_END);

		RTN_Close(rtn);
	}

}

/* ===================================================================== */

LOCALFUN VOID ImageUnload(IMG img, VOID *) {
	if(pthreadLoaded && (IMG_Id(img) == pthreadIMGID)) {
		pthreadLoaded = false;
		pthreadIMGID = 0;
	}
}

/* ===================================================================== */

int main(int argc, CHAR *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    control.CheckKnobs(Handler, 0);
    skipper.CheckKnobs(0);
    
	tls_key = PIN_CreateThreadDataKey(ThreadLocalDestruct);
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);

    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddContextChangeFunction(OnSig, 0);

    PIN_AddFiniFunction(Fini, 0);

    filter.Activate();
    icount.Activate();
    
	GLB_LOCK_INIT();

	if(gettimeofday(&startTime, NULL)) {
		printf("Failed to get the start time!");
		exit(-1);
	}

    // Never returns

    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
