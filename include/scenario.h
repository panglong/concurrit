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

#include "common.h"
#include "yield.h"

namespace counit {

class YieldImpl;

/*
 * represents a single test scenario
 * any new test must define a subclass of Scenario and override some methods: TestCase
 */

#define SC_TITLE	"{{" << name_ << "}} "

// exploration type
enum ExploreType {FORALL, EXISTS};

class Scenario {
public:
	explicit Scenario(const char* name);
	virtual ~Scenario();

	void LoadScheduleFromFile(const char* filename);

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

	TransferPoint* OnYield(SchedulePoint* spoint, Coroutine* target);

	void OnAccess(Coroutine* current, SharedAccess* access);

	void OnException(std::exception* e);

	/*
	 * Methods to be used in testcases to control the test scenario
	 */
	Coroutine* CreateThread(const char* name, ThreadEntryFunction function, void* arg);

	/* returns the same scenario for concatenating calls */
	Scenario* Until(UntilCondition* until);
	Scenario* Until(const std::string& label);
	Scenario* Until(const char* label);
	Scenario* UntilStar();
	Scenario* UntilEnd();
	Scenario* UntilFirst();

	Scenario* Except(Coroutine* t);
	Scenario* Except(const char* name);

	bool AllEnded();

	/* start running the given target */
	SchedulePoint* Transfer(Coroutine* target, SourceLocation* loc = NULL);
	SchedulePoint* TransferStar(SourceLocation* loc = NULL);

	// predefined execution templates
	void ExhaustiveSearch();
	void ContextBoundedExhaustiveSearch(int C);
	void NDSeqSearch();

	void RunSavedSchedule(const char* filename);

	inline SchedulePoint* do_yield(CoroutineGroup* group, Coroutine* current, Coroutine* target, std::string& label, SourceLocation* loc, SharedAccess* access) {
		return CHECK_NOTNULL(yield_impl_)->Yield(this, group, current, target, label, loc, access);
	}

protected:

	void RunOnce();
	void RunSetUp();
	void RunTearDown();
	void RunTestCase();

	virtual bool CheckUntil(SchedulePoint* point);

	void ResolvePoint(SchedulePoint* point);

	Coroutine* GetNextEnabled(TransferPoint* transfer = NULL);

	// update backtrack sets of transfer points for DPOR
	void UpdateBacktrackSets();

private:

	bool Backtrack();
	bool DoBacktrack();
	void Finish();
	void Restart();

	DECL_FIELD(const char*, name)
	DECL_FIELD(TransferCriteria, transfer_criteria)
	DECL_FIELD_REF(CoroutineGroup, group)
	DECL_FIELD_REF(Schedule*, schedule)
	DECL_FIELD(ExploreType, explore_type)

	DECL_FIELD(VCTracker, vcTracker)
	DECL_FIELD(YieldImpl*, yield_impl)

	friend class DefaultYieldImpl;
};

} // end namespace

#endif /* SCENARIO_H_ */
