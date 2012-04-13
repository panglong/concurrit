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
#include "transpred.h"

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
#define STAR(stmt, line) 	static StaticChoiceInfo __choice_info_##line(line); stmt(DSLChoice(&__choice_info_##line))
#define WHILE_STAR			STAR(while, __LINE__)
#define IF_STAR				STAR(if, __LINE__)
#define ELSE				else

/********************************************************************************/

#define PTRUE			TransitionPredicate::True()
#define PFALSE			TransitionPredicate::False()

/********************************************************************************/

//#define FUNC(v, f)		static FuncVar v(reinterpret_cast<void*>(f));

						// try only default, and fail if not found
#define FUNC(v, f)		static FuncVar v(FuncAddressByName(#f, true, false, true));

/********************************************************************************/

// exists thread

//#define NOPRED			TransitionPredicatePtr()

#define TVAR(t)			ThreadVarPtr t(new ThreadVar());

#define EXISTS(t, ...)	ThreadVarPtr t = DSLExistsThread(__VA_ARGS__);

#define FORALL(t, ...)	ThreadVarPtr t = DSLForallThread(__VA_ARGS__);

/********************************************************************************/

#define CONSTRAIN_ALL(pred) 	TransitionPredicatePtr __constraint_##__LINE__(new TransitionConstraintAll(pred)); ConstraintInstaller __constraint_installer_##__LINE__(this, __constraint_##__LINE__);

#define CONSTRAIN_FST(pred) 	TransitionPredicatePtr __constraint_##__LINE__(new TransitionConstraintFirst(pred)); ConstraintInstaller __constraint_installer_##__LINE__(this, __constraint_##__LINE__);

/********************************************************************************/

#define RUN_UNTIL1(r, ...) 			DSLTransferUntil((r), __VA_ARGS__);

#define RUN_UNTIL2a(p, r, ...) 		{ CONSTRAIN_FST(p); RUN_UNTIL1((r), __VA_ARGS__); }

#define RUN_UNTIL2b(q, r, ...) 		{ CONSTRAIN_ALL(q); RUN_UNTIL1((r), __VA_ARGS__); }

#define RUN_UNTIL3(p, q, r, ...) 	{ CONSTRAIN_FST(p); CONSTRAIN_ALL(q); RUN_UNTIL1((r), __VA_ARGS__); }

/********************************************************************************/

#define RUN_ONCE(p, ...)			RUN_UNTIL2a((p), (p), __VA_ARGS__);

#define RUN_UNTIL(q, r, ...) 		RUN_UNTIL2b((q), (r), __VA_ARGS__);

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


/********************************************************************************/

#define CONCURRIT_BEGIN_TEST(test_name, test_desc) \
		/* define scenario sub-class */ \
		class test_name : public Scenario { \
		public: \
			test_name() : Scenario(test_desc) {} \
			~test_name() {} \


/********************************************************************************/

#define SETUP() 	void SetUp()
#define TEARDOWN() 	void TearDown()
#define TESTCASE() 	void TestCase()

/********************************************************************************/

#define CONCURRIT_END_TEST(test_name) \
		}; /* end class */ \
		/* add to the suite */ \
		static StaticSuiteAdder<test_name> __static_suite_adder_##test_name##__(&__concurrit_suite__); \

/********************************************************************************/

#define ENDS()			safe_notnull(AuxState::Ends.get())->TP1(AuxState::Ends, true, TID)
#define ENDS2(t)		safe_notnull(AuxState::Ends.get())->TP1(AuxState::Ends, true, t)

/********************************************************************************/

#define READS()			safe_notnull(AuxState::Reads.get())->TP0(AuxState::Reads, TID)
#define WRITES()		safe_notnull(AuxState::Writes.get())->TP0(AuxState::Writes, TID)

#define READS_FROM(x)	safe_notnull(AuxState::Reads.get())->TP1(AuxState::Reads, x, TID)
#define WRITES_TO(x)	safe_notnull(AuxState::Writes.get())->TP1(AuxState::Writes, x, TID)

/********************************************************************************/

#define ENTERS(f)		safe_notnull(AuxState::Enters.get())->TP3(AuxState::Enters, f, true, TID)
#define RETURNS(f)		safe_notnull(AuxState::Returns.get())->TP3(AuxState::Returns, f, true, TID)

#define RETURNS2(f, t)	safe_notnull(AuxState::Returns.get())->TP3(AuxState::Returns, f, true, t)

/********************************************************************************/

#define IN_FUNC(f)		TPInFunc::create(f, TID)
#define TIMES_IN_FUNC(f, k) \
						safe_notnull(AuxState::NumInFunc.get())->TP3(AuxState::NumInFunc, f, k, TID)

/********************************************************************************/

#define TID				(AuxState::Tid)
#define __				(ThreadVarPtr())
//template<typename T>
//boost::shared_ptr<T> __() {
//	return boost::shared_ptr<T>();
//}
#define NOT(t)			(TID != t)
#define STEP(t)			(TID == t)

//static TransitionPredicatePtr MakeStep(ThreadVarPtr t, ...) {
//	va_list args;
//	va_start(args, t);
//	safe_assert(t != NULL);
//	TransitionPredicatePtr p = (TID == t);
//	ThreadVarPtr tt = va_arg(args, ThreadVarPtr);
//	while(tt != NULL) {
//		p = (p || (TID == tt));
//		tt = va_arg(args, ThreadVarPtr);
//	}
//	va_end(args);
//	return p;
//}
//#define STEP(...) \
//	MakeStep(__VA_ARGS__, ThreadVarPtr())

//static TransitionPredicatePtr MakeNStep(const ThreadVarPtr& t, ...) {
//	ThreadVarPtr tt;
//	va_list args;
//	va_start(args, t);
//	safe_assert(t != NULL);
//	TransitionPredicatePtr p = (TID != t);
//	tt = va_arg(args, const ThreadVarPtr&);
//	while(tt != NULL) {
//		p = (p && (TID != tt));
//		tt = va_arg(args, const ThreadVarPtr&);
//	}
//	va_end(args);
//	return p;
//}
//#define NSTEP(...) \
//	MakeNStep(__VA_ARGS__, ThreadVarPtr())


/********************************************************************************/

#define AT_PC(pc)		safe_notnull(AuxState::Pc.get())->TP1(AuxState::Pc, pc, TID)

/********************************************************************************/

// allow to add command line arguments
#define CONFIG(s)		static bool __config_##__LINE__ = Config::ParseCommandLine(StringToMainArgs((s), true));

/********************************************************************************/

#define MAX_WAIT_TIME(t)	Config::MaxWaitTimeUSecs = (t);

/********************************************************************************/

} // end namespace

#endif /* API_H_ */
