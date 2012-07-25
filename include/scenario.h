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

#ifndef SCENARIO_H_
#define SCENARIO_H_

#include <boost/shared_ptr.hpp>
#include <glog/logging.h>

#include "common.h"
#include "coroutine.h"
#include "statistics.h"
#include "group.h"
#include "transpred.h"
#include "dsl.h"

namespace concurrit {

class Result;

/*
 * represents a single test scenario
 * any new test must define a subclass of Scenario and override some methods: TestCase
 */

#define SC_TITLE	"{{" << name_ << "}} "

// exploration type
enum ExploreType {FORALL, EXISTS};
enum TestStatus { TEST_BEGIN = 0, TEST_SETUP = 1, TEST_CONTROLLED = 2, TEST_UNCONTROLLED = 3, TEST_TEARDOWN = 4, TEST_ENDED = 5, TEST_TERMINATED = 6 };

class Scenario {
public:
	explicit Scenario(const char* name);
	virtual ~Scenario();

	static Scenario* Current() {
		return Concurrit::current_scenario();
	}

	static Scenario* NotNullCurrent() {
		return CHECK_NOTNULL(Concurrit::current_scenario());
	}

	void LoadScheduleFromFile(const char* filename);

	ThreadVarPtr RunTestDriver();

	void WaitForTestStart();

	/*
	 * methods defining the test scenario, the initialization, termination, and the actual testcase
	 */
	virtual void SetUp() { /* default implementation */ };
	virtual void TearDown() { /* default implementation */ };
	virtual void TestCase() = 0;

	/*
	 * Explore a scenario and find a matching schedule
	 */
	Result* Explore();
	Result* ExploreExists();
	Result* ExploreForall();

	void CheckExists() {
		explore_type_ = EXISTS;
	}

	void CheckForall() {
		explore_type_ = FORALL;
	}

//	TransferPoint* OnYield(SchedulePoint* spoint, Coroutine* target);

//	virtual void OnAccess(Coroutine* current, SharedAccess* access);

	void OnException(std::exception* e);

	void OnSignal(int signal_number);

	void SaveSearchInfo();

	/*
	 * Methods to be used in testcases to control the test scenario
	 */
	ThreadVarPtr CreateThread(THREADID tid, ThreadEntryFunction function, void* arg = NULL, pthread_t* pid = NULL, const pthread_attr_t* attr = NULL);
	ThreadVarPtr CreateThread(ThreadEntryFunction function, void* arg = NULL, pthread_t* pid = NULL, const pthread_attr_t* attr = NULL);

	ThreadVarPtr CreatePThread(ThreadEntryFunction function, void* arg = NULL, pthread_t* pid = NULL, const pthread_attr_t* attr = NULL);

	void JoinThread(Coroutine* co, void ** value_ptr = NULL);
	void JoinPThread(Coroutine* co, void ** value_ptr = NULL);
	void JoinAllThreads(long timeout = -1);

	/* returns the same scenario for concatenating calls */
//	Scenario* Until(UntilCondition* until);
//	Scenario* Until(const std::string& label);
//	Scenario* Until(const char* label);
//	Scenario* UntilStar();
//	Scenario* UntilEnd();
//	Scenario* UntilFirst();

//	Scenario* Except(Coroutine* t);
//	Scenario* Except(THREADID tid);

	bool AllEnded();

	/* start running the given target */
//	SchedulePoint* Transfer(Coroutine* target, SourceLocation* loc = NULL);
//	SchedulePoint* TransferStar(SourceLocation* loc = NULL);

//	void RunSavedSchedule(const char* filename);

	// default yield implementation is provided by Scenario
//	virtual YIELD_SIGNATURE;

//	inline SchedulePoint* do_yield(CoroutineGroup* group, Coroutine* current, Coroutine* target, std::string& label, SourceLocation* loc, SharedAccess* access) {
//		return safe_notnull(yield_impl_)->Yield(this, group, current, target, label, loc, access);
//	}

	/******************************************************************/

	// run before and after each controlled transition
//	void BeforeControlledTransition(Coroutine* current);
//	void AfterControlledTransition(Coroutine* current);
	void OnControlledTransition(Coroutine* current);
//	void OnControlledTransition(Coroutine* current) {
//		BeforeControlledTransition(current);
//		AfterControlledTransition(current);
//	}

	/******************************************************************/

	bool DSLChoice(StaticDSLInfo* static_info, const char* message = NULL);
	bool DSLConditional(StaticDSLInfo* static_info, bool value, const char* message = NULL);

//	void DSLTransition(const TransitionPredicatePtr& assertion, const TransitionPredicatePtr& pred, const ThreadVarPtr& var = ThreadVarPtr(), const char* message = NULL);

	void DSLRunThrough(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const ThreadVarPtr& var, const char* message = NULL);
	void DSLRunThrough(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const char* message = NULL);

	void DSLRunUntil(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const ThreadVarPtr& var, const char* message = NULL);
	void DSLRunUntil(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const char* message = NULL);

	ThreadVarPtr DSLExistsThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const char* message = NULL);
	ThreadVarPtr DSLExistsThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred, const char* message = NULL);
	ThreadVarPtr DSLExistsThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred, const ThreadExprPtr& texpr, const char* message = NULL);

	ThreadVarPtr DSLForallThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const char* message = NULL);
	ThreadVarPtr DSLForallThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred, const char* message = NULL);
	ThreadVarPtr DSLForallThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred, const ThreadExprPtr& texpr, const char* message = NULL);

	void EvalSelectThread(Coroutine* current, SelectThreadNode* node, int& child_index, bool& take);
	void EvalTransition(Coroutine* current, TransitionNode* node, int& child_index, bool& take);
	void UpdateAlternateLocations(Coroutine* current);

	/******************************************************************/
	// shortcuts to statistics
	Timer& timer(const std::string& name) {
		return statistics_.timer(name);
	}

	Counter& counter(const std::string& name) {
		return statistics_.counter(name);
	}

	AvgCounter& avg_counter(const std::string& name) {
		return statistics_.avg_counter(name);
	}


protected:

	// runs the threads uncontrolled way, until they all got into ended state
	void RunUncontrolled();

	virtual ConcurritException* RunOnce() throw();
	void RunTestCase() throw();
	void RunSetUp() throw();
	void RunTearDown() throw();
	ConcurritException* CollectExceptions();

//	virtual bool CheckUntil(SchedulePoint* point);

//	void ResolvePoint(SchedulePoint* point);

//	Coroutine* GetNextEnabled(TransferPoint* transfer = NULL);

	// update backtrack sets of transfer points for DPOR
//	void UpdateBacktrackSets();


	bool Backtrack(BacktrackReason reason);
//	bool DoBacktrackCooperative(BacktrackReason reason);
//	bool DoBacktrackPreemptive(BacktrackReason reason);

	virtual void Start();
	virtual void Finish(Result* result);

//	inline bool is_replaying() { bool r = !exec_tree_.replay_path()->empty(); safe_assert(Config::TrackAlternatePaths || !r); return r; }

private:

	DECL_FIELD(const char*, name)
//	DECL_FIELD(TransferCriteria, transfer_criteria)
	DECL_FIELD_REF(CoroutineGroup, group)
	DECL_FIELD(Schedule*, schedule)
	DECL_FIELD(ExploreType, explore_type)

//	DECL_FIELD(VCTracker, vcTracker)
//	DECL_FIELD(YieldImpl*, yield_impl)

	DECL_FIELD(bool, dpor_enabled)
	DECL_VOL_FIELD(TestStatus, test_status)

	DECL_FIELD(TransitionConstraintsPtr, trans_constraints)
	DECL_FIELD(TransitionAssertionsPtr, trans_assertions)

	DECL_FIELD(Statistics, statistics)

	DECL_FIELD_GET_REF(ExecutionTreeManager, exec_tree)

	DECL_STATIC_FIELD(FILE*, trace_file)

//	DECL_FIELD_REF(Semaphore, test_end_sem)
};

/************************************************************************************/

} // end namespace

#endif /* SCENARIO_H_ */
