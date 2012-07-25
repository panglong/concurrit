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
#include "transpred.h"
#include "pinmonitor.h"
#include "threadvar.h"

namespace concurrit {

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

#define TEST_FORALL()	CheckForall()
#define TEST_EXISTS()	CheckExists()

#define DISABLE_DPOR()	set_dpor_enabled(false);

/********************************************************************************/

#define TERMINATE_SEARCH() \
	TRIGGER_TERMINATE_SEARCH()

/********************************************************************************/

// double expansion trick!
#define ADD_LINE_2(s, line)				s##line
#define ADD_LINE_1(s, line)				ADD_LINE_2(s, line)
#define ADD_LINE(s)						ADD_LINE_1(s, __LINE__)

/********************************************************************************/

// represents a set of coroutines to restrict the group to the rest of those
class WithoutThreads {
public:
	WithoutThreads(ThreadVarPtrSet* scope, ThreadVarPtrSet set) : scope_(scope), set_(set) {
		safe_check(!set.empty());
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
	DECL_FIELD(ThreadVarPtrSet*, scope)
	DECL_FIELD_REF(ThreadVarPtrSet, set)
};

/********************************************************************************/

// represents a set of coroutines to restrict the group to only those
class WithThreads {
public:
	WithThreads(ThreadVarPtrSet* scope, ThreadVarPtrSet set) {
		safe_check(!set.empty());
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

	// first check PinMonitor
	ADDRINT addr = PinMonitor::GetAddressOfSymbol(std::string(func_name), 0);
	if(addr != 0) {
		return ADDRINT2PTR(addr);
	}

	// check check current address space
	void* handle = Concurrit::driver_handle();
	if(handle == NULL) {
		handle = RTLD_DEFAULT;
	}
	void* f = FuncAddressByName(func_name, handle, false);
	if(f == NULL && handle != RTLD_DEFAULT) {
		f = FuncAddressByName(func_name, true, true, true);
	}
	safe_assert(f != NULL);
	return f;
}

#define FUNC(v, f)					static FuncVar v(#f, _FUNC(#f)); v.set_addr(_FUNC(#f));
#define FVAR(v, f)					FUNC(v, f)

/********************************************************************************/

#define TID				(AuxState::Tid)

#define ANY_THREAD		AnyThreadExpr::create()

#define MAKE_THREADVARPTRSET(...)		MakeThreadVarPtrSet(__VA_ARGS__)

/********************************************************************************/

inline ThreadExprPtr _BY(const ThreadVarPtr& t) {
	return ThreadVarExpr::create(t);
}

#define BY(t)			_BY(t)

/********************************************************************************/

inline ThreadExprPtr _TEXPR(const ThreadVarPtr& t1 = ThreadVarPtr(),
							const ThreadVarPtr& t2 = ThreadVarPtr(),
							const ThreadVarPtr& t3 = ThreadVarPtr(),
							const ThreadVarPtr& t4 = ThreadVarPtr(),
							const ThreadVarPtr& t5 = ThreadVarPtr()) {

	ThreadExprPtr p = ANY_THREAD;
	if(t1 != NULL) p = p + t1; else return p;
	if(t2 != NULL) p = p + t2; else return p;
	if(t3 != NULL) p = p + t3; else return p;
	if(t4 != NULL) p = p + t4; else return p;
	if(t5 != NULL) p = p + t5; else return p;
	return p;
}

#define TEXPR(...)		_TEXPR(__VA_ARGS__)

/********************************************************************************/

inline ThreadExprPtr _NOT_BY(const ThreadVarPtr& t) {
	return NegThreadVarExpr::create(t);
}

#define NOT_BY(t)		_NOT_BY(t)
#define NOT(t)			NOT_BY(t)

/********************************************************************************/

inline TransitionPredicatePtr _DISTINCT(ThreadVarPtrSet scope, ThreadVarPtr t = ThreadVarPtr()) {
	if(scope.empty()) return ANY_THREAD;

	if(t == NULL) t = TID;

	ThreadExprPtr e = ANY_THREAD;
	for(ThreadVarPtrSet::iterator itr = scope.begin(), end = scope.end(); itr != end; ++itr) {
		e = e - (*itr);
	}
	return e;
}

#define DISTINCT(s, ...)		_DISTINCT(MAKE_THREADVARPTRSET s, ##__VA_ARGS__)

/********************************************************************************/

// to be used in predicates defined below, when memory address accessed or function called is not important
#define ANY_ADDR		(NULL)
#define ANY_FUNC		(NULL)
#define ANY_PC			(-1)

/********************************************************************************/

#define TVAR(t)			static ThreadVarPtr t(new ThreadVar(NULL, #t));  t->clear_thread();
// /***/ ThreadVarDef __def__##t(scope(), (t))

/********************************************************************************/

// core definitions for exists and forall

#define	DECL_STATIC_SELECT_THREAD_INFO(scope, code)		static StaticSelectThreadInfo STATIC_DSL_INFO_NAME (MAKE_THREADVARPTRSET scope, RECORD_SRCLOC(), (code));

#define _EXISTS(op, t, s, ...)					DECL_STATIC_SELECT_THREAD_INFO(s, op " " #t); t << DSLExistsThread(&STATIC_DSL_INFO_NAME, (STATIC_DSL_INFO_NAME.scope()), ## __VA_ARGS__);

#define _FORALL(op, t, s, ...)					DECL_STATIC_SELECT_THREAD_INFO(s, op " " #t); t << DSLForallThread(&STATIC_DSL_INFO_NAME, (STATIC_DSL_INFO_NAME.scope()), ## __VA_ARGS__);

/********************************************************************************/

#define EXISTS(t, ...)							_EXISTS("EXISTS", t, (), ## __VA_ARGS__)

#define FORALL(t, ...)							_FORALL("FORALL", t, (), ## __VA_ARGS__)

/********************************************************************************/

#define CHOOSE_THREAD(t, s, ...)				_EXISTS("CHOOSE_THREAD", t, s, ## __VA_ARGS__)

#define CHOOSE_THREAD_BACKTRACK(t, s, ...)		_FORALL("CHOOSE_THREAD_BACKTRACK", t, s, ## __VA_ARGS__)

/********************************************************************************/

#define ASSERT_ALL(pred) 			TransitionPredicatePtr __assertion_##__LINE__(pred); AssertionInstaller __assertion_installer_##__LINE__(this, __assertion_##__LINE__);

/********************************************************************************/

#define CONSTRAIN_ALL(pred) 		TransitionPredicatePtr __constraint_##__LINE__(new TransitionConstraintAll(pred)); ConstraintInstaller __constraint_installer_##__LINE__(this, __constraint_##__LINE__);

#define CONSTRAIN_FST(pred) 		TransitionPredicatePtr __constraint_##__LINE__(new TransitionConstraintFirst(pred)); ConstraintInstaller __constraint_installer_##__LINE__(this, __constraint_##__LINE__);

/********************************************************************************/

#define _RUN_THROUGH1(r, ...) 		DECL_STATIC_DSL_INFO("RUN_THROUGH " #r); DSLRunThrough(&STATIC_DSL_INFO_NAME, (r), ## __VA_ARGS__);

#define _RUN_THROUGH2a(p, r, ...) 	{ CONSTRAIN_FST(p); _RUN_THROUGH1((r), ## __VA_ARGS__); }

#define _RUN_THROUGH2b(q, r, ...) 	{ CONSTRAIN_ALL(q); _RUN_THROUGH1((r), ## __VA_ARGS__); }

#define _RUN_THROUGH3(p, q, r, ...) { CONSTRAIN_FST(p); CONSTRAIN_ALL(q); _RUN_THROUGH1((r), ## __VA_ARGS__); }

/********************************************************************************/

#define _RUN_THROUGH(q, r, ...) 	_RUN_THROUGH2b((q), (r), ## __VA_ARGS__);

// TODO(elmas): remove later:
#define RUN_THROUGH(...)			_RUN_THROUGH(__VA_ARGS__)

/********************************************************************************/

#define _RUN_UNTIL1(r, ...) 		DECL_STATIC_DSL_INFO("RUN_UNTIL " #r); DSLRunUntil(&STATIC_DSL_INFO_NAME, (r), ## __VA_ARGS__);

#define _RUN_UNTIL2a(p, r, ...) 	{ CONSTRAIN_FST(p); _RUN_UNTIL1((r), ## __VA_ARGS__); }

#define _RUN_UNTIL2b(q, r, ...) 	{ CONSTRAIN_ALL(q); _RUN_UNTIL1((r), ## __VA_ARGS__); }

#define _RUN_UNTIL3(p, q, r, ...) 	{ CONSTRAIN_FST(p); CONSTRAIN_ALL(q); _RUN_UNTIL1((r), ## __VA_ARGS__); }

/********************************************************************************/

#define _RUN_UNTIL(q, r, ...) 		_RUN_UNTIL2b((q), (r), ## __VA_ARGS__);

// TODO(elmas): remove later:
#define RUN_UNTIL(...)				_RUN_UNTIL(__VA_ARGS__)

/********************************************************************************/

#define RUN_THREADS_THROUGH(ts, q, ...)		_RUN_THROUGH(ts, (q), ## __VA_ARGS__)
#define RUN_THREADS_UNTIL(ts, q, ...)		_RUN_UNTIL(ts, (q), ## __VA_ARGS__)

/********************************************************************************/

#define RUN_THREAD_THROUGH(t, q, ...)		_RUN_THROUGH(BY(t), (q), ## __VA_ARGS__)
#define RUN_THREAD_UNTIL(t, q, ...)			_RUN_UNTIL(BY(t), (q), ## __VA_ARGS__)

/********************************************************************************/

#define WAIT_FOR_THREAD(t, ...)				    _EXISTS("WaitForThread", t, (), ## __VA_ARGS__)

#define _WAIT_FOR_DISTINCT_THREADS(ts, p, ...) \
		ThreadVarPtrSet ADD_LINE(s) = (ts); \
		ThreadExprPtr ADD_LINE(q) = ANY_THREAD; \
		for(ThreadVarPtrSet::iterator ADD_LINE(itr) = ADD_LINE(s).begin(), ADD_LINE(end) = ADD_LINE(s).end(); ADD_LINE(itr) != ADD_LINE(end); ++ADD_LINE(itr)) { \
			ThreadVarPtr ADD_LINE(t) = (*ADD_LINE(itr)); \
			_EXISTS("WaitForDistinctThread", ADD_LINE(t), (), ((p) && ADD_LINE(q)), ## __VA_ARGS__); \
			ADD_LINE(q) = ADD_LINE(q) - ADD_LINE(t); \
		} \

#define WAIT_FOR_DISTINCT_THREADS(ts, ...)	_WAIT_FOR_DISTINCT_THREADS(MAKE_THREADVARPTRSET ts, ## __VA_ARGS__)

/********************************************************************************/

// select and release

#define SELECT(t, p, ...)	WAIT_FOR_THREAD(t, p, ## __VA_ARGS__)

#define RELEASE(t, ...)		RUN_THREAD_THROUGH(t, PTRUE, ## __VA_ARGS__)

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
#define CALL_TEST(name)		MYLOG(1) << "Calling testcase " #name; TestCase_##name()

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
			&& safe_notnull(AuxState::Pc.get())->TP1(AuxState::Pc, pc, t);
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

inline ADDRINT _ARG0(ThreadVarPtr& t, ADDRINT f = 0) {
	if(f == 0) {
		return (safe_notnull(AuxState::Arg0.get())->get_first_value(safe_notnull(t.get())->tid()));
	} else {
		return (safe_notnull(AuxState::Arg0.get())->get((f), safe_notnull(t.get())->tid()));
	}
}

#define ARG0(...)			_ARG0(__VA_ARGS__)

/********************************************************************************/

inline ADDRINT _ARG1(ThreadVarPtr& t, ADDRINT f = 0) {
	if(f == 0) {
		return (safe_notnull(AuxState::Arg1.get())->get_first_value(safe_notnull(t.get())->tid()));
	} else {
		return (safe_notnull(AuxState::Arg1.get())->get((f), safe_notnull(t.get())->tid()));
	}
}

#define ARG1(...)			_ARG1(__VA_ARGS__)

/********************************************************************************/

inline ADDRINT _RETVAL(ThreadVarPtr& t, ADDRINT f = 0) {
	if(f == 0) {
		return (safe_notnull(AuxState::RetVal.get())->get_first_value(safe_notnull(t.get())->tid()));
	} else {
		return (safe_notnull(AuxState::RetVal.get())->get((f), safe_notnull(t.get())->tid()));
	}
}

#define RETVAL(...)		_RETVAL(__VA_ARGS__)

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

inline bool HAVE_ENDED(ThreadVarPtr t1,
					   ThreadVarPtr t2 = ThreadVarPtr(),
					   ThreadVarPtr t3 = ThreadVarPtr(),
					   ThreadVarPtr t4 = ThreadVarPtr(),
					   ThreadVarPtr t5 = ThreadVarPtr()) {
	if(!HAS_ENDED(t1)) return false;
	if(t2 != NULL && !HAS_ENDED(t2)) return false;
	if(t3 != NULL && !HAS_ENDED(t3)) return false;
	if(t4 != NULL && !HAS_ENDED(t4)) return false;
	if(t5 != NULL && !HAS_ENDED(t5)) return false;
	return true;
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
