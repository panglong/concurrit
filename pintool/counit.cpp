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

/************************************************/

#include "pin.H"
#include "instlib.H"
#include "portability.H"
#include <assert.h>
#include <stdio.h>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <assert.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <iostream>
#include <sstream>

/************************************************/

#include "vc.h"

/************************************************/

#define LOCK_FOR_SHARED

#include "pin_base.cpp" // code that does instrumentation

/************************************************/

static void PIN_FAST_ANALYSIS_CALL ThreadCreate(VOID* addr, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL ThreadJoin(ADDRINT addr, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL LockRelease(ADDRINT value, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL LockAcquire(ADDRINT value, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL RWLockRelease(ADDRINT value, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL RLockAcquire(ADDRINT value, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL WLockAcquire(ADDRINT value, THREADID tid, ADDRINT pc) {}
static void PIN_FAST_ANALYSIS_CALL ProcedureCall(ADDRINT pc, ADDRINT addr, THREADID tid) {}
static void PIN_FAST_ANALYSIS_CALL ProcedureReturn(ADDRINT pc, ADDRINT addr, THREADID tid) {}

/************************************************/

static vctime_t glbTime = 1;
static std::map<ADDRINT, VC> glbVC;
static std::map<ADDRINT, std::vector<SharedAccess*>*> glbLastAccesses;

static std::vector<SharedAccess*>* GetLastAccesses(ADDRINT mem) {
	std::vector<SharedAccess*>* accesses = NULL;
	std::map<ADDRINT, std::vector<SharedAccess*>*>::iterator itr = glbLastAccesses.find(mem);
	if(itr != glbLastAccesses.end()) {
		accesses = itr->second;
	}
	return accesses;
}

static void UpdateLastAccesses(SharedAccess* current) {
	std::vector<SharedAccess*>* accesses = NULL;
	std::map<ADDRINT, std::vector<SharedAccess*>*>::iterator itr = glbLastAccesses.find(current->mem());
	if(itr != glbLastAccesses.end()) {
		accesses = itr->second;
		assert(accesses != NULL);
		assert(!accesses->empty());
		SharedAccess* access = accesses->front();
		assert(access->mem() == current->mem());
		// if conflicting access, clear last accesses
		if(conflicting(access->type(), current->type())) {
			for(std::vector<SharedAccess*>::iterator it = accesses->begin();
					it < accesses->end();
					it = accesses->erase(it)) {
				access = (*it);
				assert(access->mem() == current->mem());
				delete access;
			}
			assert(accesses->empty());
		}
	} else {
		accesses = new std::vector<SharedAccess*>();
		glbLastAccesses[current->mem()] = accesses;
	}
	accesses->push_back(current);
}

/************************************************/

// represents run of a thread between two yield points
class Strand {

};

/************************************************/

static void ReportImmediateRace(SharedAccess* last, SharedAccess* current){
	printf("Immediate race between %s and %s\n", last->ToString().c_str(), current->ToString().c_str());
}

static void PIN_FAST_ANALYSIS_CALL StrandStart(VOID* name, THREADID tid, ADDRINT pc) {
	printf("PIN: Begin stride: %s thread id %d...\n", static_cast<const char*>(name), tid);
}

/************************************************/

static void PIN_FAST_ANALYSIS_CALL OnSharedAccess(ADDRINT pc, ADDRINT mem, THREADID tid, AccessType type)
{
	ThreadLocalState* tls = GetThreadLocalState(tid);
	assert(tls != NULL);
	assert(tls->tid() == tid);

	SharedAccess* current = new SharedAccess(tid, type, mem, pc, glbTime);

	// find immediate races
	std::vector<SharedAccess*>* accesses = GetLastAccesses(mem);
	if(accesses != NULL) {

		VC vcTid = tls->vc();

		// iterate over last accesses
		for(std::vector<SharedAccess*>::iterator it = accesses->begin();
				it < accesses->end();
				it++)
		{
			SharedAccess* access = (*it);
			assert(access->mem() == mem);
			if(!conflicting(access->type(), type)){
				break;
			}
			// check immediate race
			if(access->tid() != current->tid()) {
				if(!(access->time() <= vc_get(vcTid, access->tid()))) {
					// immediate race between access and current access
					ReportImmediateRace(access, current);
				}
			}
		}

		// update vector clocks
		VC vcMem = vc_get_vc(glbVC, mem);

		VC vcNew = vc_cup(vcMem, vcTid);
		vc_set(vcNew, tid, glbTime);

		tls->set_vc(vcNew);
		vc_set_vc(glbVC, mem, vcNew);

	}

	// update global accesses
	UpdateLastAccesses(current);

	glbTime++;

	// SourceLocation loc(pc);
	// printf("Thread %d executing %s\n", tls->tid(), loc.ToString().c_str());
}


static void PIN_FAST_ANALYSIS_CALL SharedReadAccess(ADDRINT pc, ADDRINT mem, THREADID tid)
{
	OnSharedAccess(pc, mem, tid, READ_ACCESS);
}

static void PIN_FAST_ANALYSIS_CALL SharedWriteAccess(ADDRINT pc, ADDRINT mem, THREADID tid)
{
	OnSharedAccess(pc, mem, tid, WRITE_ACCESS);
}

static void PIN_FAST_ANALYSIS_CALL LockForShared(THREADID tid)
{
	GLB_LOCK();
}

static void PIN_FAST_ANALYSIS_CALL UnlockForShared(THREADID tid)
{
	GLB_UNLOCK();
}

VOID Fini(INT32 code, VOID *v) {
	if(gettimeofday(&endTime, NULL)) {
		printf("Failed to get the end time!");
		exit(-1);
	}

	timeval_subtract(&runningTime, &endTime, &startTime);

	printf("\nTIME for program:\t%lu seconds, %lu microseconds\n", runningTime.tv_sec, runningTime.tv_usec);

	PIN_DeleteThreadDataKey(tls_key);
}

int main(int argc, char * argv[]) {

	if (PIN_Init(argc, argv)) return -1;

	PIN_InitSymbols();

	tls_key = PIN_CreateThreadDataKey(ThreadLocalDestruct);
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	PIN_AddFiniFunction(Fini, 0);

	TRACE_AddInstrumentFunction(TraceAnalysisCalls, 0);
	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);

	filter.Activate();

	GLB_LOCK_INIT();

	if(gettimeofday(&startTime, NULL)) {
		printf("Failed to get the start time!");
		exit(-1);
	}

	PIN_StartProgram();

	return 0;
}
