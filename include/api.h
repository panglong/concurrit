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

// double expansion trick!
#define ADD_LINE_2(s, line)				s##line
#define ADD_LINE_1(s, line)				ADD_LINE_2(s, line)
#define ADD_LINE(s)						ADD_LINE_1(s, __LINE__)

/********************************************************************************/

// constructing coroutine sets from comma-separated arguments
static inline
ThreadVarPtrSet MakeThreadVarPtrSet(ThreadVarPtr t, ...) {
	va_list args;
	ThreadVarPtrSet set;
	va_start(args, t);
	while (t != NULL) {
	   set.insert(t);
	   t = va_arg(args, ThreadVarPtr);
	}
	va_end(args);
	return set;
}
// use the following instead of MakeCoroutinePtrSet alone
#define MAKE_THREADVARPTRSET(...) \
	MakeThreadVarPtrSet(__VA_ARGS__, ThreadVarPtr())

/********************************************************************************/

// represents a set of coroutines to restrict the group to the rest of those
class WithoutThreads {
public:
	WithoutThreads(ThreadVarScope* scope, ThreadVarPtrSet set) : scope_(scope), set_(set) {
		CHECK(!set.empty());
		for(ThreadVarPtrSet::iterator itr = set_.begin(), end = set_.end(); itr != end; ++itr) {
			scope_->Remove(*itr);
		}
	}

	~WithoutThreads() {
		for(ThreadVarPtrSet::iterator itr = set_.begin(), end = set_.end(); itr != end; ++itr) {
			scope_->Add(*itr);
		}
	}
private:
	DECL_FIELD(ThreadVarScope*, scope)
	DECL_FIELD_REF(ThreadVarPtrSet, set)
};

/********************************************************************************/

// represents a set of coroutines to restrict the group to only those
class WithThreads {
public:
	WithThreads(ThreadVarScope* scope, ThreadVarPtrSet set) {
		CHECK(!set.empty());
		ThreadVarPtrSet others = *scope;
		for(ThreadVarPtrSet::iterator itr = set.begin(), end = set.end(); itr != end; ++itr) {
			others.erase(*itr);
		}
		without_ = new WithoutThreads(scope, others); // takes out others
	}

	~WithThreads() {
		delete without_; // puts back others
	}

private:
	DECL_FIELD_REF(WithoutThreads*, without)
};

/********************************************************************************/

/* restricting a block of code to a set of thread variables
 * usage:
 * { WITH(s);
 *   ....
 * }
 * { WITHOUT(s);
 *   ....
 * }
 */

#define WITH(...) \
	WithThreads ADD_LINE(__withthreads__)(scope(), MAKE_THREADVARPTRSET(__VA_ARGS__))

#define WITHOUT(...) \
	WithoutThreads ADD_LINE(__withoutthreads__)(scope(), MAKE_THREADVARPTRSET(__VA_ARGS__))

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

#define STATIC_DSL_INFO_NAME			ADD_LINE(__static_dsl_info_)
#define	DECL_STATIC_DSL_INFO(code) 		static StaticDSLInfo STATIC_DSL_INFO_NAME (RECORD_SRCLOC(), (code));

/********************************************************************************/

#define STAR(nd, stmt, cond, code) 	static StaticChoiceInfo STATIC_DSL_INFO_NAME((nd), (RECORD_SRCLOC()), (code)); stmt((cond) && DSLChoice(&STATIC_DSL_INFO_NAME))

#define WHILE_AND_STAR(c)			STAR(Config::IsStarNondeterministic, while, (c), ("WHILE_AND_STAR(" #c ")"))
#define IF_AND_STAR(c)				STAR(Config::IsStarNondeterministic, if, (c), ("IF_AND_STAR(" #c ")"))

#define WHILE_STAR					STAR(Config::IsStarNondeterministic, while, true, "WHILE_STAR")
#define IF_STAR						STAR(Config::IsStarNondeterministic, if, true, "IF_STAR")

#define WHILE_DTSTAR				STAR(false, while, true, "WHILE_DTSTAR")
#define IF_DTSTAR					STAR(false, if, true, "IF_DTSTAR")

#define WHILE_NDSTAR				STAR(true, while, true, "WHILE_NDSTAR")
#define IF_NDSTAR					STAR(true, if, true, "IF_NDSTAR")

#define ELSE						else
#define DO							do

/********************************************************************************/

#define COND(stmt, cond, code) 		DECL_STATIC_DSL_INFO(code); stmt(DSLConditional(&STATIC_DSL_INFO_NAME, (cond)))

#define WHILE(cond)					COND(while, (cond), "WHILE("#cond")")
#define IF(cond)					COND(if, (cond), "IF("#cond")")

/********************************************************************************/

// for loops until some condition (e.g., all coroutines end)
#define ALL_ENDED		(AllEnded())
#define UNTIL_ALL_END	WHILE(!ALL_ENDED)
#define UNTIL_ENDS(c)	WHILE(!(c)->is_ended())

/********************************************************************************/

#define PTRUE						TransitionPredicate::True()
#define PFALSE						TransitionPredicate::False()

/********************************************************************************/

inline void* _FUNC(const char* func_name) {
	void* handle = Concurrit::driver_handle();
	if(handle == NULL) {
		handle = RTLD_DEFAULT;
	}
	void* f = FuncAddressByName(func_name, handle, true);
	safe_assert(f != NULL);
	return f;
}

#define FUNC(v, f)					static FuncVar v(#f, _FUNC(#f));
#define FVAR(v, f)					FUNC(v, f)

/********************************************************************************/

#define TID				(AuxState::Tid)
#define __				(ThreadVarPtr())

/********************************************************************************/

inline TransitionPredicatePtr _BY(ThreadVarPtr t, ...) {
	va_list args;
	va_start(args, t);
	safe_assert(t != NULL);
	TransitionPredicatePtr p;
	do {
		TransitionPredicatePtr q = (TID == t);
		p = ((p == NULL) ? q : (p || q));
		t = va_arg(args, ThreadVarPtr);
	} while (t != NULL);
	va_end(args);
	return p;
}

#define BY(t)			(TID == t)
//#define BY(...)			_BY(__VA_ARGS__, ThreadVarPtr())

/********************************************************************************/

inline TransitionPredicatePtr _NOT_BY(ThreadVarPtr t, ...) {
	va_list args;
	va_start(args, t);
	safe_assert(t != NULL);
	TransitionPredicatePtr p;
	do {
		TransitionPredicatePtr q = (TID != t);
		p = ((p == NULL) ? q : (p && q));
		t = va_arg(args, ThreadVarPtr);
	} while (t != NULL);
	va_end(args);
	return p;
}

#define NOT_BY(t)			(TID != t)
//#define NOT_BY(...)			_NOT_BY(__VA_ARGS__, ThreadVarPtr())
#define NOT(...)			NOT_BY(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _DISTINCT(ThreadVarScope* scope, ThreadVarPtr t = ThreadVarPtr()) {
	if(scope->empty()) return PTRUE;

	if(t == NULL) t = TID;

	TransitionPredicatePtr p = TransitionPredicate::True();
	for(ThreadVarScope::iterator itr = scope->begin(), end = scope->end(); itr != end; ++itr) {
		p = p && (t != (*itr));
	}
	return p;
}

#define DISTINCT(...)		_DISTINCT(scope(), __VA_ARGS__)

/********************************************************************************/

// to be used in predicates defined below, when memory address accessed or function called is not important
#define ANY_ADDR		(NULL)
#define ANY_FUNC		(NULL)
#define ANY_PC			(-1)

/********************************************************************************/

#define TVAR(t)			static ThreadVarPtr t(new ThreadVar()); /***/ ThreadVarDef __def__##t(scope(), (t))

/********************************************************************************/

// core definitions for exists and forall

#define _EXISTS(op, t, ...)		DECL_STATIC_DSL_INFO(op " " #t); TVAR(t); t << DSLExistsThread(&STATIC_DSL_INFO_NAME, __VA_ARGS__);

#define _FORALL(op, t, ...)		DECL_STATIC_DSL_INFO(op " " #t); TVAR(t); t << DSLForallThread(&STATIC_DSL_INFO_NAME, __VA_ARGS__);

/********************************************************************************/

#define EXISTS(t, ...)			_EXISTS("EXISTS", t, NULL, __VA_ARGS__)

#define FORALL(t, ...)			_FORALL("FORALL", t, NULL, __VA_ARGS__)

/********************************************************************************/

#define WAIT_FOR_THREAD(t, ...)					_EXISTS("WAIT_FOR_THREAD", t, NULL, __VA_ARGS__)

#define WAIT_FOR_DISTINCT_THREAD(t, p, ...)		_EXISTS("WAIT_FOR_DISTINCT_THREAD", t, NULL, ((p) && DISTINCT(TID)), __VA_ARGS__)

/********************************************************************************/

#define SELECT_THREAD(t, ...)					_EXISTS("SELECT_THREAD", t, scope(), __VA_ARGS__)

#define SELECT_THREAD_BACKTRACK(t, ...)			_FORALL("FORALL_THREAD", t, scope(), __VA_ARGS__)

/********************************************************************************/

#define ASSERT_ALL(pred) 			TransitionPredicatePtr __assertion_##__LINE__(pred); AssertionInstaller __assertion_installer_##__LINE__(this, __assertion_##__LINE__);

/********************************************************************************/

#define CONSTRAIN_ALL(pred) 		TransitionPredicatePtr __constraint_##__LINE__(new TransitionConstraintAll(pred)); ConstraintInstaller __constraint_installer_##__LINE__(this, __constraint_##__LINE__);

#define CONSTRAIN_FST(pred) 		TransitionPredicatePtr __constraint_##__LINE__(new TransitionConstraintFirst(pred)); ConstraintInstaller __constraint_installer_##__LINE__(this, __constraint_##__LINE__);

/********************************************************************************/

#define RUN_UNTIL1(r, ...) 			DECL_STATIC_DSL_INFO("RUN_UNTIL " #r); DSLTransferUntil(&STATIC_DSL_INFO_NAME, (r), __VA_ARGS__);

#define RUN_UNTIL2a(p, r, ...) 		{ CONSTRAIN_FST(p); RUN_UNTIL1((r), __VA_ARGS__); }

#define RUN_UNTIL2b(q, r, ...) 		{ CONSTRAIN_ALL(q); RUN_UNTIL1((r), __VA_ARGS__); }

#define RUN_UNTIL3(p, q, r, ...) 	{ CONSTRAIN_FST(p); CONSTRAIN_ALL(q); RUN_UNTIL1((r), __VA_ARGS__); }

/********************************************************************************/

#define RUN_ONCE(p, ...)			RUN_UNTIL2a((p), (p), __VA_ARGS__);

#define RUN_UNTIL(q, r, ...) 		RUN_UNTIL2b((q), (r), __VA_ARGS__);

/********************************************************************************/

#define RUN_UNLESS1(r, ...) 		DECL_STATIC_DSL_INFO("RUN_UNLESS " #r); DSLTransferUnless(&STATIC_DSL_INFO_NAME, (r), __VA_ARGS__);

#define RUN_UNLESS2a(p, r, ...) 	{ CONSTRAIN_FST(p); RUN_UNLESS1((r), __VA_ARGS__); }

#define RUN_UNLESS2b(q, r, ...) 	{ CONSTRAIN_ALL(q); RUN_UNLESS1((r), __VA_ARGS__); }

#define RUN_UNLESS3(p, q, r, ...) 	{ CONSTRAIN_FST(p); CONSTRAIN_ALL(q); RUN_UNLESS1((r), __VA_ARGS__); }

/********************************************************************************/

#define RUN_UNLESS(q, r, ...) 		RUN_UNLESS2b((q), (r), __VA_ARGS__);

/********************************************************************************/

#define RUN_THREAD_UNTIL(t, q, ...)		RUN_UNTIL(BY(t), (q), __VA_ARGS__)
#define RUN_THREAD_UNLESS(t, q, ...)	RUN_UNLESS(BY(t), (q), __VA_ARGS__)


/********************************************************************************/

class ConcurritInitializer {
public:
	ConcurritInitializer(int argc = -1, char **argv = NULL) {
		Concurrit::Init(argc, argv);
	}
	~ConcurritInitializer() {
		Concurrit::Destroy();
	}
};

/********************************************************************************/

#define CONCURRIT_BEGIN_MAIN() \
		using namespace concurrit; \
		static Suite __concurrit_suite__; \


#define CONCURRIT_END_MAIN() \
		int main(int argc, char ** argv) { \
			static ConcurritInitializer __concurrit__initializer__(argc, argv); \
			system("clear"); \
			__concurrit_suite__.RunAll(); \
			safe_exit(EXIT_SUCCESS); \
			return EXIT_SUCCESS; \
		} \


/********************************************************************************/

#define CONCURRIT_BEGIN_TEST(test_name, test_desc) \
		/* define scenario sub-class */ \
		class test_name : public DefaultScenario { \
		public: \
			test_name() : DefaultScenario(test_desc) {} \
			~test_name() {} \


/********************************************************************************/

#define SETUP() 			void SetUp()
#define TEARDOWN() 			void TearDown()
#define TESTCASE() 			void TestCase()
#define TEST(name) 			void TestCase_##name()
#define CALL_TEST(name)		TestCase_##name()

/********************************************************************************/

#define CONCURRIT_END_TEST(test_name) \
		}; /* end class */ \
		/* add to the suite */ \
		static StaticSuiteAdder<test_name> __static_suite_adder_##test_name##__(&__concurrit_suite__); \

/********************************************************************************/

inline TransitionPredicatePtr _ENDS(ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	return safe_notnull(AuxState::Ends.get())->TP1(AuxState::Ends, true, t);
}

#define ENDS(...)		_ENDS(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _READS(void* x = NULL, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(x == NULL)
		return safe_notnull(AuxState::Reads.get())->TP0(AuxState::Reads, t);
	else
		return safe_notnull(AuxState::Reads.get())->TP1(AuxState::Reads, PTR2ADDRINT(x), t);
}

#define READS(...)		_READS(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _WRITES(void* x = NULL, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(x == NULL)
		return safe_notnull(AuxState::Writes.get())->TP0(AuxState::Writes, t);
	else
		return safe_notnull(AuxState::Writes.get())->TP1(AuxState::Writes, PTR2ADDRINT(x), t);
}

#define WRITES(...)		_WRITES(__VA_ARGS__)

/********************************************************************************/

#define ACCESSES(...)	(READS(__VA_ARGS__) || WRITES(__VA_ARGS__))

#define TO_READ(...)	READS(__VA_ARGS__)
#define TO_WRITES(...)	WRITES(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _ENTERS(void* f = NULL, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(f == NULL)
		return safe_notnull(AuxState::Enters.get())->TP0(AuxState::Enters, t);
	else
		return safe_notnull(AuxState::Enters.get())->TP3(AuxState::Enters, PTR2ADDRINT(f), true, t);
}

#define ENTERS(...)		_ENTERS(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _RETURNS(void* f = NULL, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(f == NULL)
		return safe_notnull(AuxState::Returns.get())->TP0(AuxState::Returns, t);
	else
		return safe_notnull(AuxState::Returns.get())->TP3(AuxState::Returns, PTR2ADDRINT(f), true, t);
}

#define RETURNS(...)	_RETURNS(__VA_ARGS__)

/********************************************************************************/

#define TO_ENTER(...)	ENTERS(__VA_ARGS__)
#define TO_RETURN(...)	RETURNS(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _CALLS(void* f = NULL, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(f == NULL)
		return safe_notnull(AuxState::CallsTo.get())->TP0(AuxState::CallsTo, t);
	else
		return safe_notnull(AuxState::CallsTo.get())->TP3(AuxState::CallsTo, PTR2ADDRINT(f), true, t);
}

#define CALLS(...)		_CALLS(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _IN_FUNC(void* f, ThreadVarPtr t = ThreadVarPtr(), int k = 1) {
	if(t == NULL) t = TID;
	safe_assert(k >= 1);
	if(k == 1) {
		return TPInFunc::create(PTR2ADDRINT(f), t);
	} else {
		return safe_notnull(AuxState::NumInFunc.get())->TP3(AuxState::NumInFunc, PTR2ADDRINT(f), k, t);
	}
}

#define IN_FUNC(...)	_IN_FUNC(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _AT_PC(int pc = -1, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(pc < 0)
		return safe_notnull(AuxState::Pc.get())->TP0(AuxState::Pc, t);
	else
		return safe_notnull(AuxState::Pc.get())->TP1(AuxState::Pc, pc, t);
}

#define AT_PC(...)		_AT_PC(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _HITS_PC(int pc = -1, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	if(pc < 0)
		return safe_notnull(AuxState::AtPc.get())->TP0(AuxState::AtPc, t);
	else
		return safe_notnull(AuxState::AtPc.get())->TP0(AuxState::AtPc, t)
			&& safe_notnull(AuxState::AtPc.get())->TP1(AuxState::AtPc, pc, t);
}

#define HITS_PC(...)	_HITS_PC(__VA_ARGS__)

/********************************************************************************/

typedef AuxConst0<ADDRINT, ADDRINT(0)> AuxVar0_ADDRINT;
typedef boost::shared_ptr<AuxVar0_ADDRINT> AuxVar0_ADDRINT_PTR;
#define AVAR(x)		AuxVar0_ADDRINT_PTR x(new AuxVar0_ADDRINT(ADDRINT(0)));

/********************************************************************************/

inline TransitionPredicatePtr _WITH_ARG0(void* f, const AuxVar0_ADDRINT_PTR& arg0, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	return safe_notnull(AuxState::Arg0.get())->TP5(AuxState::Arg0, PTR2ADDRINT(f), arg0, t);
}

inline TransitionPredicatePtr _WITH_ARG0(void* f, ADDRINT arg0, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	return safe_notnull(AuxState::Arg0.get())->TP3(AuxState::Arg0, PTR2ADDRINT(f), arg0, t);
}

#define WITH_ARG0(...)		_WITH_ARG0(__VA_ARGS__)

/********************************************************************************/

inline TransitionPredicatePtr _WITH_ARG1(void* f, const AuxVar0_ADDRINT_PTR& arg1, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	return safe_notnull(AuxState::Arg1.get())->TP5(AuxState::Arg1, PTR2ADDRINT(f), arg1, t);
}

inline TransitionPredicatePtr _WITH_ARG1(void* f, ADDRINT arg1, ThreadVarPtr t = ThreadVarPtr()) {
	if(t == NULL) t = TID;
	return safe_notnull(AuxState::Arg1.get())->TP3(AuxState::Arg1, PTR2ADDRINT(f), arg1, t);
}

#define WITH_ARG1(...)		_WITH_ARG1(__VA_ARGS__)

/********************************************************************************/

// read current aux-state

#define ARG0(t, f)			(safe_notnull(AuxState::Arg0.get())->get((f), safe_notnull(t.get())->tid()))
#define ARG1(t, f)			(safe_notnull(AuxState::Arg1.get())->get((f), safe_notnull(t.get())->tid()))

/********************************************************************************/

// allow to add command line arguments
#define CONFIG(s)		static bool __config_##__LINE__ = Config::ParseCommandLine(StringToMainArgs((s), true));

/********************************************************************************/

#define MAX_WAIT_TIME(t)	Config::MaxWaitTimeUSecs = (t);

/********************************************************************************/

inline bool HAS_ENDED(ThreadVarPtr t) {
	safe_assert(t != NULL && !t->is_empty());
	return t->thread()->is_ended();
}

/********************************************************************************/

// wait for all threads to end
#define WAIT_FOR_ALL(...)		JoinAllThreads(__VA_ARGS__)

// wait for a particular thread to end
inline void _WAIT_FOR_END(ThreadVarPtr t, long timeout = -1) {
	safe_assert(t != NULL && !t->is_empty());
	t->thread()->WaitForEnd(timeout);
}

#define WAIT_FOR_END(...)	_WAIT_FOR_END(__VA_ARGS__)

/********************************************************************************/

} // end namespace

#endif /* API_H_ */
