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

#define DECL_FIELD(type, name) \
		protected: \
		type name##_; \
		public: \
		inline type& name() { return name##_; } \
		inline void set_##name(type value) { name##_ = value; } \
		private:	\

/************************************************/
// global lock for analysis code

static PIN_LOCK lock;
#define GLB_LOCK_INIT()		InitLock(&lock)
#define GLB_LOCK()			GetLock(&lock, 1)
#define GLB_UNLOCK()		ReleaseLock(&lock)

/************************************************/

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

/************************************************/

struct timeval startTime;
struct timeval endTime;
struct timeval runningTime;

/************************************************/

static void PIN_FAST_ANALYSIS_CALL StrandStart(VOID* name, THREADID tid, ADDRINT pc);

static void PIN_FAST_ANALYSIS_CALL ThreadCreate(VOID* addr, THREADID tid, ADDRINT pc);
static void PIN_FAST_ANALYSIS_CALL ThreadJoin(ADDRINT addr, THREADID tid, ADDRINT pc);
static void PIN_FAST_ANALYSIS_CALL SharedReadAccess(ADDRINT pc, ADDRINT ea, THREADID tid);
static void PIN_FAST_ANALYSIS_CALL SharedWriteAccess(ADDRINT pc, ADDRINT ea, THREADID tid);
static void PIN_FAST_ANALYSIS_CALL LockForShared(THREADID tid);
static void PIN_FAST_ANALYSIS_CALL UnlockForShared(THREADID tid);
static void PIN_FAST_ANALYSIS_CALL LockRelease(ADDRINT value, THREADID tid, ADDRINT pc);
static void PIN_FAST_ANALYSIS_CALL LockAcquire(ADDRINT value, THREADID tid, ADDRINT pc);
static void PIN_FAST_ANALYSIS_CALL ProcedureCall(ADDRINT pc, ADDRINT addr, THREADID tid);
static void PIN_FAST_ANALYSIS_CALL ProcedureReturn(ADDRINT pc, ADDRINT addr, THREADID tid);

static void PIN_FAST_ANALYSIS_CALL RWLockRelease(ADDRINT value, THREADID tid, ADDRINT pc);
static void PIN_FAST_ANALYSIS_CALL RLockAcquire(ADDRINT value, THREADID tid, ADDRINT pc);
static void PIN_FAST_ANALYSIS_CALL WLockAcquire(ADDRINT value, THREADID tid, ADDRINT pc);

/************************************************/

class SourceLocation {
public:
	SourceLocation(ADDRINT address) {
		Update(address);
	}

	void Update(ADDRINT address) {
		filename_ = "<unknown>";
		column_ = -1;
		line_ = -1;

		PIN_LockClient();
		PIN_GetSourceLocation(address, &column_, &line_, &filename_);
		PIN_UnlockClient();
	}


	std::string ToString() {
		std::stringstream s;
		if(IsUnknown()) {
			s << "<unknown>";
		} else {
			s << "(" << filename_ << " # " << line_ << " : " << column_ << ")";
		}
		return s.str();
	}

	bool IsUnknown() {
		bool r = (line_ == 0);
		assert(!r || column_ == 0);
		assert(!r || filename_ == "");
		return r;
	}

	bool operator ==(const SourceLocation& loc) const {
		return (column_ == loc.column_) && (line_ == loc.line_) && (filename_ == loc.filename_);
	}

	bool operator !=(const SourceLocation& loc) const {
		return !(operator==(loc));
	}

private:
	DECL_FIELD(std::string, filename)
	DECL_FIELD(INT32, column)
	DECL_FIELD(INT32, line)
};

/************************************************/

enum AccessType {WRITE_ACCESS, READ_ACCESS};

bool conflicting(AccessType t1, AccessType t2) {
	return (t1 == WRITE_ACCESS) || (t2 == WRITE_ACCESS);
}

class SharedAccess {
public:
	SharedAccess(THREADID tid, AccessType type, ADDRINT mem, ADDRINT pc, vctime_t time)
	{
		Update(tid, type, mem, pc, NULL, time);
	}

	void Update(THREADID tid, AccessType type, ADDRINT mem, ADDRINT pc, SourceLocation* loc, vctime_t time) {
		time_ = time;
		tid_ = tid;
		type_ = type;
		mem_ = mem;
		pc_ = pc;
		loc_ = loc;
	}

	~SharedAccess() {
		if(loc_ != NULL) {
			delete loc_;
		}
	}

	SourceLocation* source_location() {
		if(loc_ == NULL) {
			loc_ = new SourceLocation(pc_);
		}
		return loc_;
	}

	std::string ToString() {
		std::stringstream s;
		s << (type_ == READ_ACCESS ? "Read from " : "Write to ") << mem_ <<" by " << tid_ << " at " << time_ << " location " << source_location()->ToString();
		return s.str();
	}

private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD(AccessType, type)
	DECL_FIELD(ADDRINT, mem)
	DECL_FIELD(ADDRINT, pc)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(vctime_t, time)
};

/************************************************/

TLS_KEY tls_key;

class ThreadLocalState {
public:
	ThreadLocalState(THREADID tid) {
		printf("Creating thread local state for tid %d...\n", tid);

		assert(PIN_IsApplicationThread());
		assert(tid != INVALID_THREADID);
		assert(tid == PIN_ThreadId());

		tid_ = tid;
		vc_.clear();
	}

	~ThreadLocalState() {
		printf("Deleting thread local state for tid %d...\n", tid_);
	}

private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD(VC, vc)
};

ThreadLocalState* GetThreadLocalState(THREADID tid) {
	assert(tid == PIN_ThreadId());
	VOID* ptr = PIN_GetThreadData(tls_key, tid);
	assert(ptr != NULL);
	ThreadLocalState* tls = static_cast<ThreadLocalState*>(ptr);
	assert(tls->tid() == tid);
	return tls;
}

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    GLB_LOCK();

    assert(threadid == PIN_ThreadId());

    printf("Thread %d starting...\n", threadid);

    PIN_SetThreadData(tls_key, new ThreadLocalState(threadid), threadid);

    GLB_UNLOCK();
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
	assert(threadid == PIN_ThreadId());

	printf("Thread %d ending...\n", threadid);
}

VOID ThreadLocalDestruct(VOID* ptr) {
	if(ptr != NULL) {
		ThreadLocalState* o = static_cast<ThreadLocalState*>(ptr);
		delete o;
	}
}


/************************************************/

INSTLIB::FILTER filter;

/************************************************/

void TraceAnalysisCalls(TRACE trace, void *) {

	if (!filter.SelectTrace(trace))
		return;

	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {

		for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {

			if (INS_IsOriginal(ins)) {
				if(!INS_IsStackRead(ins)
					&& !INS_IsStackWrite(ins) && (INS_IsMemoryRead(ins)
					|| INS_IsMemoryWrite(ins) || INS_HasMemoryRead2(ins))) {

#ifdef LOCK_FOR_SHARED
					// acquire the global lock to serialize the access
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(LockForShared),
							IARG_FAST_ANALYSIS_CALL, IARG_THREAD_ID, IARG_END);
#endif

					// Log every memory references of the instruction
					if (INS_IsMemoryRead(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(
								SharedReadAccess),
								IARG_FAST_ANALYSIS_CALL, IARG_INST_PTR,
								IARG_MEMORYREAD_EA, IARG_THREAD_ID, IARG_END);
					}
					if (INS_IsMemoryWrite(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(
								SharedWriteAccess),
								IARG_FAST_ANALYSIS_CALL, IARG_INST_PTR,
								IARG_MEMORYWRITE_EA, IARG_THREAD_ID, IARG_END);
					}
					if (INS_HasMemoryRead2(ins)) {
						INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(
								SharedReadAccess),
								IARG_FAST_ANALYSIS_CALL, IARG_INST_PTR,
								IARG_MEMORYREAD2_EA, IARG_THREAD_ID, IARG_END);
					}

#ifdef LOCK_FOR_SHARED
					// release the global lock
					if (INS_HasFallThrough(ins)) {
						INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(UnlockForShared),
								IARG_FAST_ANALYSIS_CALL, IARG_THREAD_ID, IARG_END);
					}

					if (INS_IsBranchOrCall(ins)) {
						INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(
								UnlockForShared), IARG_FAST_ANALYSIS_CALL,
								IARG_THREAD_ID, IARG_END);
					}
#endif
				}

#if CALLS_ENABLED
				if(INS_IsProcedureCall(ins)) {
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(
							ProcedureCall),
							IARG_FAST_ANALYSIS_CALL, IARG_INST_PTR,
							IARG_BRANCH_TARGET_ADDR, IARG_THREAD_ID, IARG_END);
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(
							ProcedureReturn),
							IARG_FAST_ANALYSIS_CALL, IARG_INST_PTR,
							IARG_BRANCH_TARGET_ADDR, IARG_THREAD_ID, IARG_END);
				}
#endif
			}

		}
	}

}

static bool pthreadLoaded = false;
static UINT32 pthreadIMGID = 0;

VOID ImageLoad(IMG img, VOID *) {

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

VOID ImageUnload(IMG img, VOID *) {
	if(pthreadLoaded && IMG_Id(img) == pthreadIMGID) {
		pthreadLoaded = false;
		pthreadIMGID = 0;
	}
}
