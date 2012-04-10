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
#include "yieldapi.h"
#include "exception.h"
#include "coroutine.h"

namespace concurrit {

#define ENDING_LABEL "ending"
#define MAIN_LABEL "main"

/********************************************************************************/

#define CREATE_THREAD \
	CreateThread

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
//extern void BeginStrand(const char* name);

/********************************************************************************/

// DSL for the preemptive mode
//#define STAR	DSLChoice(__LINE__)
#define STAR(stmt, line) static StaticChoiceInfo __choice_info_##line(line); stmt(DSLChoice(&__choice_info_##line))
#define WHILE_STAR	STAR(while, __LINE__)
#define IF_STAR		STAR(if, __LINE__)
#define ELSE		else

#define CONSTRAIN_ALL(pred) \
	TransitionConstraintAll(this, (pred))

#define CONSTRAIN_FIRST(pred) \
	TransitionConstraintFirst(this, (pred))

/********************************************************************************/

// exists thread

#define EXISTS(t)	ThreadVarPtr t = ThreadVarPtr(new ThreadVar()); \
					DSLExistsThread(t);

#define EXISTS_ST(t, p) \
					ThreadVarPtr t = ThreadVarPtr(new ThreadVar()); \
					DSLExistsThread(t, p);

#define FORALL(t)	ThreadVarPtr t = ThreadVarPtr(new ThreadVar()); \
					DSLForallThread(t);

/********************************************************************************/

#define FUNC(v, f)	static FuncVar v(reinterpret_cast<void*>(f))

/********************************************************************************/

#define RUN_UNTIL(t, p)		DSLTransferUntil((t), (p))
#define RUN_ONCE(t, p)		DSLTransition((p), (t))
#define TRANSITION(p)		DSLTransition((p))

/********************************************************************************/

#define CONCURRIT_BEGIN_MAIN() \
		using namespace concurrit; \
		static Suite __concurrit_suite__; \


#define CONCURRIT_END_MAIN() \
		int main(int argc, char ** argv) { \
			static ConcurritInitializer __concurrit__initializer__(argc, argv); \
			__concurrit_suite__.RunAll(); \
			return EXIT_SUCCESS; \
		} \


#define CONCURRIT_BEGIN_TEST(test_name, test_desc) \
		/* define scenario sub-class */ \
		class test_name : public Scenario { \
		public: \
			test_name() : Scenario(test_desc) {} \
			~test_name() {} \


#define SETUP() 	void SetUp()
#define TEARDOWN() 	void TearDown()
#define TESTCASE() 	void TestCase()


#define CONCURRIT_END_TEST(test_name) \
		}; /* end class */ \
		/* add to the suite */ \
		static StaticSuiteAdder<test_name> __static_suite_adder_##test_name##__(&__concurrit_suite__); \


} // end namespace

#endif /* API_H_ */
