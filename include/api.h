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

#ifndef API_H_
#define API_H_

#include "common.h"
#include "sharedaccess.h"
#include "exception.h"
#include "coroutine.h"

namespace concurrit {

#define ENDING_LABEL "ending"
#define MAIN_LABEL "main"

/********************************************************************************/

#define CREATE_THREAD \
	CreateThread

/********************************************************************************/

/*
 * API to be used by the test cases
 */

#define RECORD_SRCLOC()		(new SourceLocation(__FILE__, __FUNCTION__, __LINE__))

/********************************************************************************/

class SchedulePoint;
extern SchedulePoint* yield(const char* label, SourceLocation* loc = NULL, SharedAccess* access = NULL, bool force = false);

/********************************************************************************/

#define NO_ACCESS NULL

#define READ(x) \
	new SharedAccess(READ_ACCESS, new MemoryCell<T>(x), #x)

#define WRITE(x) \
	new SharedAccess(WRITE_ACCESS, new MemoryCell<T>(x), #x)

#define YIELD(label, access) \
	yield(label, RECORD_SRCLOC(), access)

#define FORCE_YIELD(label, access) \
	yield(label, RECORD_SRCLOC(), access, /*force=*/true)

/********************************************************************************/
// yielding reads and writes

template<typename T>
class writer {
public:
	explicit writer(const char* label, SharedAccess* access, SourceLocation* loc, bool yield_before = true)
	: label_(label), access_(access), loc_(loc), yield_before_(yield_before) {}

	T& operator =(const T& value) {
		MemoryCell<T>* cell = access_->cell_as<T>();

		// we set the value of the memory cell even before the yield,
		// but do not perform the access yet
		// so yield implementation can access the value even before the actual access
		cell->set_value(value);

		if(yield_before_) yield(label_, loc_, access_);

		// perform the actual access
		T& val = cell->write(value);

		// notify scenario about this access
		NOTNULL(Coroutine::Current())->OnAccess(access_);

		if(!yield_before_) yield(label_, loc_, access_);

		return val;
	}
private:
	DECL_FIELD(const char*, label)
	DECL_FIELD(SharedAccess*, access)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(bool, yield_before)
};

template<typename T>
class reader {
public:
	explicit reader(const char* label, SharedAccess* access, SourceLocation* loc, bool yield_before = true)
	: label_(label), access_(access), loc_(loc), yield_before_(yield_before) {}

	operator T() {
		MemoryCell<T>* cell = access_->cell_as<T>();

		// allows us to load the value to cell.value
		cell->read();

		if(yield_before_) yield(label_, loc_, access_);

		// perform the actual access
		T val = cell->read();

		// notify scenario about this access
		NOTNULL(Coroutine::Current())->OnAccess(access_);

		if(!yield_before_) yield(label_, loc_, access_);

		return val;
	}
private:
	DECL_FIELD(const char*, label)
	DECL_FIELD(SharedAccess*, access)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(bool, yield_before)
};

template<typename T>
inline reader<T> yield_read(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(READ_ACCESS, new MemoryCell<T>(mem), expr);
	return reader<T>(label, access, loc, /*yield_before=*/ true); // makes the yield on T()
}

template<typename T>
inline reader<T> read_yield(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(READ_ACCESS, new MemoryCell<T>(mem), expr);
	return reader<T>(label, access, loc, /*yield_before=*/ false); // makes the yield on T()
}

template<typename T>
inline writer<T> yield_write(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(WRITE_ACCESS, new MemoryCell<T>(mem), expr);
	return writer<T>(label, access, loc, /*yield_before=*/ true); // makes the yield on operator=()
}

template<typename T>
inline writer<T> write_yield(const char* label, T* mem, const char* expr, SourceLocation* loc) {
	SharedAccess* access = new SharedAccess(WRITE_ACCESS, new MemoryCell<T>(mem), expr);
	return writer<T>(label, access, loc, /*yield_before=*/ false); // makes the yield on operator=()
}

#define YIELD_READ(label, x) \
	yield_read(label, (&(x)), #x, RECORD_SRCLOC())

#define YIELD_WRITE(label, x) \
	yield_write(label, (&(x)), #x, RECORD_SRCLOC())

#define READ_YIELD(label, x) \
	read_yield(label, (&(x)), #x, RECORD_SRCLOC())

#define WRITE_YIELD(label, x) \
	write_yield(label, (&(x)), #x, RECORD_SRCLOC())


/********************************************************************************/


#define Assert(cond)	\
	{ if (!(cond)) { throw new AssertionViolationException(#cond, RECORD_SRCLOC()); } }

#define Assume(cond)	\
	{ if (!(cond)) { throw new AssumptionViolationException(#cond, RECORD_SRCLOC()); } }

#define ASSERT(p) \
	Assert(p)

#define ASSUME(p) \
	Assume(p)

/********************************************************************************/

#define TRANSFER(t) \
	Transfer(t, RECORD_SRCLOC())

#define TRANSFER_STAR() \
	TransferStar(RECORD_SRCLOC())

/********************************************************************************/

#define UNTIL(x) \
	Until(x)

#define UNTIL_TRUE(pred, trans) \
	{ trans ; Assume(pred); }

#define UNTIL_FALSE(pred, trans) \
	{ trans ; Assume(!(pred)); }

#define UNTIL_FIRST() \
	UntilFirst()

#define UNTIL_END() \
	UntilEnd()

#define UNTIL_STAR() \
	UntilStar()

/********************************************************************************/

// exclude a target from transfer
#define EXCEPT(t) \
	Except(t)

/********************************************************************************/

// for loops until some condition (e.g., all coroutines end)
#define DO				do
#define ALL_ENDED		(AllEnded())
#define UNTIL_ALL_END	while(!ALL_ENDED)
#define UNTIL_ENDS(c)	while(!(c)->is_ended())

/********************************************************************************/

#define TEST_FORALL()	CheckForall()
#define TEST_EXISTS()	CheckExists()

#define DISABLE_DPOR()	set_dpor_enabled(false);

/********************************************************************************/

#define TERMINATE_SEARCH() \
	TRIGGER_TERMINATE_SEARCH()

/********************************************************************************/

#define EXHAUSTIVE_SEARCH() \
	ExhaustiveSearch()

#define CONTEXT_BOUNDED_EXHAUSTIVE_SEARCH(c) \
	ContextBoundedExhaustiveSearch(c)

#define NDSEQ_SEARCH() \
	NDSeqSearch()

#define FINISH(t) \
	UNTIL_END()->TRANSFER(t)

#define FINISH_STAR() \
	UNTIL_END()->TRANSFER_STAR()

#define FINISH_ALL() \
	UNTIL_ALL_END { \
		FINISH_STAR(); \
	}

#define RUN_SAVED_SCHEDULE(f) \
	RunSavedSchedule(f)

/********************************************************************************/

// constructing coroutine sets from comma-separated arguments
static CoroutinePtrSet MakeCoroutinePtrSet(Coroutine* co, ...) {
	va_list args;
	CoroutinePtrSet set;
	va_start(args, co);
	while (co != NULL) {
	   set.insert(co);
	   co = va_arg(args, Coroutine*);
	}
	va_end(args);
	return set;
}
// use the following instead of MakeCoroutinePtrSet alone
#define MAKE_COROUTINEPTRSET(...) \
	MakeCoroutinePtrSet(__VA_ARGS__, NULL)

/********************************************************************************/

/* restricting a block of code to a set of coroutines
 * usage:
 * { WITH(s);
 *   ....
 * }
 * { WITHOUT(s);
 *   ....
 * }
 */
#define WITH(...) \
	WithGroup __withgroup__(group(), MAKE_COROUTINEPTRSET(__VA_ARGS__))

#define WITHOUT(...) \
	WithoutGroup __withoutgroup__(group(), MAKE_COROUTINEPTRSET(__VA_ARGS__))

/********************************************************************************/

/* delaying and resuming coroutines
 * usage:
 * DELAY(t);
 * ....
 * RESUME(t);
 */
#define DELAY(t) \
	t->MakeDelayed()

#define RESUME(t) \
	t->CancelDelayed()

/********************************************************************************/

// this is used for the pin instrumentation
extern void BeginStrand(const char* name);

/********************************************************************************/

extern void BeginCounit();
extern void EndCounit();


} // end namespace

#endif /* API_H_ */
