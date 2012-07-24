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

#include "concurrit.h"

namespace concurrit {

/********************************************************************************/
/*
 * Scenario
 */

FILE* Scenario::trace_file_ = NULL;

//UntilStarCondition TransferCriteria::until_star_;
//UntilFirstCondition TransferCriteria::until_first_;
//UntilEndCondition TransferCriteria::until_end_;

Scenario::Scenario(const char* name) {
	schedule_ = NULL;

	name_ = safe_notnull(name);

	// schedule and group are initialized
	group_.set_scenario(this);
	explore_type_ = FORALL;

//	transfer_criteria_.Reset();

	// Scenario provides the default yield implementation
//	yield_impl_ = static_cast<YieldImpl*>(this);

	dpor_enabled_ = true;

	TransitionConstraintsPtr p(new TransitionConstraints());
	trans_constraints_ = p;

	TransitionAssertionsPtr q(new TransitionAssertions());
	trans_assertions_ = q;

	test_status_ = TEST_BEGIN;
}

/********************************************************************************/

Scenario::~Scenario() {
	if(schedule_ != NULL) {
		delete schedule_;
	}
}

/********************************************************************************/

void Scenario::LoadScheduleFromFile(const char* filename) {
	CHECK(filename != NULL) << "Filename is NULL!";
	if(schedule_ == NULL) {
		schedule_ = new Schedule();
	}
	MYLOG(2) << SC_TITLE << "Loading schedule from file " << filename;
	schedule_->LoadFromFile(filename);
	MYLOG(2) << SC_TITLE << "Loaded schedule from file " << filename;
}

/********************************************************************************/

ThreadVarPtr Scenario::RunTestDriver() {
	if(Config::TestLibraryFile != NULL) {
		MYLOG(2) << "Calling driver's main function.";

		ThreadVarPtr var = CreateThread(Concurrit::CallDriverMain, NULL);
		safe_assert(var != NULL && !var->is_empty());
//		var->thread()->set_is_driver_thread(true);

		// wait for the loading of the test library
		Concurrit::sem_driver_load()->Wait();

		return var;
	}
	MYLOG(2) << "No test-supplied __main__ function, skipping the driver thread.";
	return ThreadVarPtr();
}

/********************************************************************************/

void Scenario::WaitForTestStart() {
	// wait for start_test signal
	int result = Concurrit::sem_test_start()->WaitTimed(60*USECSPERSEC);
	if(result == ETIMEDOUT) {
		fprintf(stderr, "No TestStart event has been received! Ending the search.\n");
		TRIGGER_TERMINATE_SEARCH();
	}
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

ThreadVarPtr Scenario::CreateThread(ThreadEntryFunction function, void* arg /*= NULL*/, pthread_t* pid /*= NULL*/, const pthread_attr_t* attr /*= NULL*/) {
	return CreateThread(-1, function, arg, pid, attr);
}

// create a new thread or fetch the existing one, and start it.
ThreadVarPtr Scenario::CreateThread(THREADID tid, ThreadEntryFunction function, void* arg /*= NULL*/, pthread_t* pid /*= NULL*/, const pthread_attr_t* attr /*= NULL*/) {
	ScopeMutex smutex(group_.create_mutex());

	CHECK(tid < 0 || tid > MAIN_TID) << "CreateThread is given invalid tid: " << tid;

	// checks if the thread is expected next (!NULL), or if this is a new thread (NULL)
	Coroutine* co = NULL;
//	if(Config::TrackAlternatePaths && is_replaying()) {
//		if(tid > MAIN_TID) {
//			// use tid to return coroutine
//			safe_assert(group_.HasMember(tid));
//			co = group_.GetMember(tid);
//		} else {
//			// this is only for the first created thread (driver-thread)!
//			co = group_.GetNthCreatedMember(0, tid);
//		}
//		safe_assert(co != NULL);
//		return co->tvar();
//	}

	//================================================

	co = group_.GetNextCreatedMember(tid);

	if(co == NULL) {
		// create a new thread
		MYLOG(2) << SC_TITLE << "Creating new coroutine " << tid;
		// create and add new thread
		co = new Coroutine(tid, function, arg);
		group_.AddMember(co); // also sets the tid
	} else {
		MYLOG(2) << SC_TITLE << "Re-creating and restarting coroutine " << tid;
		safe_assert(co->status() > PASSIVE);
		// update the function and arguments
		co->set_entry_function(function);
		co->set_entry_arg(arg);
	}
	safe_assert(co->tid() > MAIN_TID);
	// start it. in the usual case, waits until a transfer happens, or starts immediatelly depending ont he argument transfer_on_start
	co->Start(pid, attr);

	return co->tvar();
}

/********************************************************************************/

// create a new thread or fetch the existing one, and start it.
ThreadVarPtr Scenario::CreatePThread(ThreadEntryFunction function, void* arg /*= NULL*/, pthread_t* pid /*= NULL*/, const pthread_attr_t* attr /*= NULL*/) {
	ScopeMutex smutex(group_.create_mutex());

	// checks if the thread is expected next (!NULL), or if this is a new thread (NULL)
	Coroutine* co = group_.GetNextCreatedMember();

	if(co == NULL) {
		// create a new thread
		MYLOG(2) << SC_TITLE << "Creating new pthread coroutine.";
		// create and add new thread
		co = new Coroutine(-1, function, arg);
		group_.AddMember(co); // also sets the tid
	} else {
		MYLOG(2) << SC_TITLE << "Re-creating and restarting pthread coroutine " << co->tid();
		safe_assert(co->status() > PASSIVE);
		// update the function and arguments
		co->set_entry_function(function);
		co->set_entry_arg(arg);
	}
	safe_assert(co != NULL && co->tid() > MAIN_TID);
	// start it. in the usual case, waits until a transfer happens, or starts immediatelly depending ont he argument transfer_on_start
	co->Start(pid, attr);

	MYLOG(2) << SC_TITLE << "Created new pthread coroutine " << co->tid();

	return co->tvar();
}

/********************************************************************************/

void Scenario::JoinThread(Coroutine* co, void ** value_ptr /*= NULL*/) {
	safe_assert(co != NULL);
//	if(Config::TrackAlternatePaths && is_replaying()) {
//		__pthread_errno__ = PTH_SUCCESS;
//
//		if(value_ptr != NULL) {
//			*value_ptr = co->return_value();
//		}
//		return;
//	}
	// call this only when not replaying
	JoinPThread(co, value_ptr);
}

/********************************************************************************/

void Scenario::JoinPThread(Coroutine* co, void ** value_ptr /*= NULL*/) {
	safe_assert(co != NULL);
	co->WaitForEnd(0); // timeout == 0 -- wait indefinitely
	safe_assert(co->is_ended());

	__pthread_errno__ = PTH_SUCCESS;

	if(value_ptr != NULL) {
		*value_ptr = co->return_value();
	}
}

/********************************************************************************/

void Scenario::JoinAllThreads(long timeout /*= -1*/) {
	if(Config::CancelThreadsToRestart) {
		// wait main thread
		Coroutine* driver = group_.GetMember(THREADID(1));
		driver->WaitForEnd();
		driver->Finish();
		// cancel others
		group_.CancelJoinAll();
		return;
	}

	//=======================================================

	if(timeout == -1) timeout = (Config::RunUncontrolled ? 0 : Config::MaxWaitTimeUSecs);

	int num_timed_out = -1; // set to -1 to start with 0 in the first iteration.
	do {
		if(exec_tree_.ENDNODE()->exception()->get_non_backtrack() != NULL) {
			MYLOG(1) << "There is a non-backtrack exception, so exiting without waiting threads!";
			break;
		}
		++num_timed_out;
		if(num_timed_out == Config::MaxTimeOutsBeforeDeadlock) {
			MYLOG(1) << "There is a deadlock, so exiting without waiting threads!";
			exec_tree_.ENDNODE()->add_exception(new DeadlockException(), Coroutine::Current(), "JoinAllThreads");
			break;
		}
	} while(group_.WaitForAllEnd(timeout) > 0);
}

/********************************************************************************/

void Scenario::OnException(std::exception* e) {
	throw e; // Explore() will catch and handle this
}

/********************************************************************************/

Result* Scenario::ExploreExists() {
	explore_type_ = EXISTS;
	return Explore();
}

/********************************************************************************/

Result* Scenario::ExploreForall() {
	explore_type_ = FORALL;
	return Explore();
}

/********************************************************************************/

Result* Scenario::Explore() {

	Result* result = new ForallResult();

	counter("Num replay fails").reset();

	for(;true;) {

		/************************************************************************/
		for(;true;) {
			try {

				MYLOG(1) << "Exploring new execution...";

				ConcurritException* exc = RunOnce();

				// RunOnce should not throw an exception, so throw it here
				if(exc != NULL) {
					MYLOG(2) << "Throwing exception from RunOnce: " << exc->what();
					throw exc;
				}

				counter("Num successful executions explored").increment();

				if(explore_type_ == EXISTS) {
					if(result != NULL) delete result;
					result = new ExistsResult(schedule_->Clone());
					break;
				} else {
					if(result == NULL) {
						result = new ForallResult();
					}
					safe_assert(INSTANCEOF(result, ForallResult*));

					ASINSTANCEOF(result, ForallResult*)->AddSchedule(schedule_->Clone());
					MYLOG(2) << "Found one path for forall search.";
					TRIGGER_BACKTRACK(SUCCESS, true);
				}

			} catch(ConcurritException* ce) {
				AssertionViolationException* ae = NULL;
				InternalException* ie = NULL;
				BacktrackException* be = NULL;

				// check the contents of ce, check backtrack last
				if((ae = ce->get_assertion_violation()) != NULL){
					MYLOG(2) << SC_TITLE << "Assertion is violated!";
					MYLOG(2) << ae->what();

					result = new AssertionViolationResult(ae, schedule_->Clone());

				} else if((ie = ce->get_internal()) != NULL){
					MYLOG(2) << SC_TITLE << "Internal exception thrown!";
					MYLOG(2) << ie->what();

					throw ie;

				} else if((be = ce->get_backtrack()) == NULL){
					MYLOG(2) << SC_TITLE << "Exception caught: " << be->what();

					result = new RuntimeExceptionResult(be, schedule_->Clone());
				} else {
					safe_assert(be != NULL);
					// Backtrack exception!!!
					safe_assert(be->reason() != UNKNOWN);

					MYLOG(2) << SC_TITLE << "Backtracking: " << be->what();

					// is backtrack is for terminating the search, exit the search loop
					if((Config::ExitOnFirstExecution == 0) || ((--Config::ExitOnFirstExecution) == 0) || (be->reason() == SEARCH_ENDS)) {
						goto LOOP_DONE; // break the outermost loop
					}

					// if uncontrolled run, then do not check backtracking
					if(Config::RunUncontrolled) break;

					if(Backtrack(be->reason())) {
						continue;
					} else {
						if(result == NULL) {
							MYLOG(2) << SC_TITLE << "No feasible execution!!!";
							result = new NoFeasibleExecutionResult(schedule_->Clone());
						} else {
							MYLOG(2) << SC_TITLE << "No more feasible execution!!!";
							safe_assert(result != NULL && INSTANCEOF(result, ForallResult*));
						}
						goto LOOP_DONE; // break the outermost loop
					}

				}
				break;
			}
			catch(std::exception* e) {
				safe_fail("Exceptions other than ConcurritException are not expected: %s!!!\n", safe_notnull(e->what()));
			}
			catch(...) {
				safe_fail("Exceptions other than std::exception are not expected!!!\n");
			}

		} // end for
		/************************************************************************/
		safe_assert(result != NULL);
		if(!INSTANCEOF(result, ForallResult*)){
			break;
		}
		safe_assert(explore_type_ == FORALL);
	} // end for

LOOP_DONE:

	MYLOG(2) << "End of exploration, calling Finish.";

	safe_assert(result != NULL);
	Finish(result); // deletes schedule_

	return result;
}


/********************************************************************************/

ConcurritException* Scenario::CollectExceptions() {
	return safe_notnull(exec_tree_.ENDNODE())->exception();
}

/********************************************************************************/

ConcurritException* Scenario::RunOnce() throw() {

	Timer timer;
	timer.start();

	Start();

	RunSetUp();

	RunTestCase();

	RunUncontrolled();

	RunTearDown();

	test_status_ = TEST_ENDED;

	timer.stop();

	avg_counter("Average time to explore each path").increment((long int)(timer.getElapsedTimeInMicroSec()));

	avg_counter("Average memory usage (KB)").increment(Statistics::GetMemoryUsageInKB());

	return CollectExceptions();
}

/********************************************************************************/

void Scenario::RunUncontrolled() {
	// set uncontrolled run flag
	test_status_ = TEST_UNCONTROLLED;

	PinMonitor::Disable();

	// this is used to signal the server thread in the server mode
//	test_end_sem_.Signal();

	//---------------------------

	Thread::Yield(true);

	MYLOG(2) << "Starting uncontrolled run";

	// start waiting all to end
	JoinAllThreads();

	MYLOG(2) << "Ending uncontrolled run";
}

/********************************************************************************/

/**
 * Functions to run setup, teardown and testcase and get the exception.
 */
void Scenario::RunSetUp() throw() {
	test_status_ = TEST_SETUP;
	try {
		SetUp();
	} catch(...) {
		safe_fail("Exceptions in SetUp are not allowed!!!\n");
	}
}

/********************************************************************************/

void Scenario::RunTearDown() throw() {
	test_status_ = TEST_TEARDOWN;
	try {
		TearDown();
	} catch(...) {
		safe_fail("Exceptions in TearDown are not allowed!!!\n");
	}
}

/********************************************************************************/

void Scenario::RunTestCase() throw() {
	test_status_ = TEST_CONTROLLED;

//	PinMonitor::Enable(); // this is performed by StartTest instrumentation callback

	//---------------------------

	BacktrackReason reason = SUCCESS;
	BacktrackException* be = NULL;
	do {
		reason = SUCCESS;
		try {

			MYLOG(1) << "(Re)Running test case...";

			RunTestDriver();

			// run test case when RunUncontrolled flag is not set
			if(!Config::RunUncontrolled) {
				// first wait for the TestStart event
				WaitForTestStart();

				// then run the actual test case
				TestCase();

				MYLOG(2) << "TestCase ended!";
			}

		} catch(std::exception* e) {
			safe_assert(e != NULL);
			// EndWithSuccess may ignore some backtracks and choose to replay
			be = ASINSTANCEOF(e, BacktrackException*);
			if(be == NULL) {
				MYLOG(1) << "Test execution ended with non-backtrack exception: " << safe_notnull(e->what());

				// mark the end of the path with end node and the corresponding exception
				exec_tree_.EndWithException(Coroutine::main(), e);
				return;
			}
			reason = be->reason();
			safe_assert(reason != SUCCESS);
			MYLOG(2) << "RunTestCase throw backtrack exception, handling with reason " << BacktrackException::ReasonToString(reason);
			// if backtrack due to timeout (at some place) and all threads have ended meanwhile,
			// then change the backtrack type to THREADS_ALLENDED
			if(reason == TIMEOUT && group_.IsAllEnded()) {
				reason = THREADS_ALLENDED;
				MYLOG(2) << "Replacing TIMEOUT with THREADS_ALLENDED; all threads have ended.";
			}
			if(reason == TIMEOUT ||
				reason == REPLAY_FAILS ||
				reason == TREENODE_COVERED ||
				reason == ASSUME_FAILS ||
				reason == THREADS_ALLENDED) { // TODO(elmas): we can find without restarting which ones win here
				// retry...
			} else {
				safe_assert(reason != SUCCESS);
				break; // this is handled below (if(reason != SUCCESS) case)
			}
		} catch(...) {
			safe_fail("Exceptions other than std::exception in TestCase are not allowed!!!\n");
		}

		MYLOG(1) << "Test script ended with backtrack: " << BacktrackException::ReasonToString(reason);

	} while(exec_tree_.EndWithSuccess(&reason));

	MYLOG(2) << "Handled all paths or there is an exception, exiting RunTestCase...";

	//====================================
	// after trying all alternate paths
//	safe_assert(exec_tree_.current_nodes()->empty() || exec_tree_.ROOTNODE()->covered());

	if(reason != SUCCESS) {
		if(be == NULL) {
			be = GetBacktrackException(reason);
		} else {
			be->set_reason(reason);
		}
		MYLOG(1) << "Test execution ended with backtrack exception: " << BacktrackException::ReasonToString(reason);

		// mark the end of the path with end node and the corresponding exception
		exec_tree_.EndWithException(Coroutine::main(), be);
	}
}

/********************************************************************************/

//void Scenario::ResolvePoint(SchedulePoint* point) {
//		YieldPoint* ypoint = point->AsYield();
//		if(!ypoint->IsResolved()) {
//			std::string* strsource = reinterpret_cast<std::string*>(ypoint->source());
//			safe_assert(strsource != NULL);
//			Coroutine* source = group_.GetMember(atoi(strsource->c_str()));
//			safe_assert(source != NULL);
//			ypoint->set_source(source);
//			ypoint->set_is_resolved(true);
//			delete strsource;
//		}
//		if(point->IsTransfer()) {
//			TransferPoint* transfer = point->AsTransfer();
//			if(!transfer->IsResolved()) {
//				std::string* strtarget = reinterpret_cast<std::string*>(transfer->target());
//				safe_assert(strtarget != NULL);
//				Coroutine* target = group_.GetMember(atoi(strtarget->c_str()));
//				safe_assert(target != NULL);
//				transfer->set_target(target);
//				transfer->set_is_resolved(true);
//				delete strtarget;
//			}
//		}
//	}

/********************************************************************************/

//bool Scenario::DoBacktrackCooperative(BacktrackReason reason) {
//	MYLOG(2) << "Start schedule: " << schedule_->ToString();
//
//	// remove all points after current (inclusively).
//	schedule_->RemoveCurrentAndBeyond();
//	for(SchedulePoint* point = schedule_->RemoveLast(); point != NULL; point = schedule_->RemoveLast()) {
//
//		if(point->IsChoice()) {
//			// check if there is any more choice
//			if(point->AsChoice()->ChooseNext()) {
//				schedule_->AddLast(point);
//
//				MYLOG(2) << "End schedule: " << schedule_->ToString();
//				return true;
//			}
//		}
//		else if(point->IsTransfer()) {
//			TransferPoint* transfer = point->AsTransfer();
//			safe_assert(transfer->IsResolved());
//
//			// if already backtrack point, decrease the count
//			if(transfer->target() == NULL) {
//				safe_assert(transfer->AsYield()->free_target());
//				if(transfer->yield()->free_count()) {
//					unsigned int count = transfer->yield()->count();
//					if(--count > 0) {
//						transfer->yield()->set_count(count);
//						schedule_->AddLast(transfer);
//
//						MYLOG(2) << "End schedule: " << schedule_->ToString();
//						return true;
//					}
//				}
//			} else if(transfer->AsYield()->free_target()) {
//				safe_assert(!transfer->enabled()->empty());
//				// if it has still more choices, make it backtrack point
//				if(transfer->has_more_targets(dpor_enabled_)) {
//					// makes this a backtrack point (adds target to done and makes target NULL)
//					transfer->make_backtrack_point();
//					schedule_->AddLast(transfer);
//
//					MYLOG(2) << "End schedule: " << schedule_->ToString();
//					return true;
//				}
//			}
//		//************************
//		// this is not a transfer point, but a plain yield point
//		} else {
//			safe_assert(point->AsYield()->count() > 0);
//
//			Coroutine* target = NULL;
//			if(!point->AsYield()->free_target()) {
//				safe_assert(point->AsYield()->label() != "MAIN" && !point->AsYield()->source()->IsMain());
//				target = group_.main();
//			}
//
//			// create a transferpoint
//			TransferPoint* transfer = new TransferPoint(point->AsYield(), target);
//
//			ResolvePoint(transfer);
//			schedule_->AddLast(transfer);
//			return true;
//		}
//		// delete the thrown-away yield point
//		MYLOG(2) << "Throwing away yield point: " << point->ToString();
//		delete point;
//	} // end loop
//	return false; // no feasible execution
//}

/********************************************************************************/

// although Start and Restart have the same code,
// we do not directly call Restart, since subclasses may override Start and Restart differently
void Scenario::Start() {
	// mark this as current scenario
	safe_assert(Scenario::Current() == NULL || Scenario::Current() == this);
	Concurrit::set_current_scenario(this);
	safe_assert(Scenario::Current() == this);

	// sanity checks for initialization
	safe_assert(Coroutine::main() != NULL);
	safe_assert(test_status_ == TEST_BEGIN || test_status_ == TEST_ENDED);

	srand(time(NULL));

	if(test_status_ == TEST_BEGIN) {
		// first start
		// reset statistics
//		statistics_.Reset();
		timer("Search time").start();
	} else {
		// restart
		exec_tree_.Restart();

		// clear aux state
		AuxState::Clear();
	}

	test_status_ = TEST_BEGIN;

//	test_end_sem_.Init(0);

	// reset counters per execution
	counter("Num Threads").reset();
	counter("Num Events").reset();

	counter("Num Execs").increment();
	fprintf(stderr, "\n\n\n\n\nEXPLORING EXECUTION -- %d -- \n\n\n\n\n", counter("Num Execs").value());

	if(schedule_ == NULL) {
		schedule_ = new Schedule();
	} else {
		schedule_->Restart();
	}

	group_.Restart(); // restarts only already started coroutines

	// reset vc tracker
//	vcTracker_.Restart();

//	transfer_criteria_.Reset();

	trans_constraints_->clear();
	trans_assertions_->clear();

	// open trace file
	if(Config::SaveExecutionTraceToFile) {
		if(trace_file_ != NULL) {
			my_fclose(trace_file_, EXIT_ON_FAIL);
			trace_file_ = NULL;
		}
		static std::string trace_file_name = InConcurritWorkDir("trace.txt");
		trace_file_ = my_fopen(safe_notnull(trace_file_name.c_str()), "w", EXIT_ON_FAIL);
		safe_assert(trace_file_ != NULL);
	}
}

/********************************************************************************/

void Scenario::Finish(Result* result) {
	safe_assert(result != NULL);
	safe_assert((INSTANCEOF(result, SignalResult*) && test_status_ < TEST_TERMINATED) || test_status_ == TEST_ENDED);
	test_status_ = TEST_TERMINATED;

	// finish timer
	timer("Search time").stop();

	if(schedule_ != NULL) {
		delete schedule_;
		schedule_ = NULL;
	}

	// clear aux state
	AuxState::Clear();

	// save information to file
	SaveSearchInfo();

	if(INSTANCEOF(result, SignalResult*)) {
		Coroutine* current = Coroutine::Current();
		group_.KillAll(ASINSTANCEOF(result, SignalResult*)->signal_number(),
					current == NULL ? 0 : current->tid());

		// print statistics as we are dying
		std::cerr << "********** Statistics **********" << std::endl;
		std::cerr << statistics_.ToString() << std::endl;
	} else {
		// otherwise, copy statistics to the result
		result->set_statistics(statistics_);

		// terminate all the threads in the group
		group_.Finish();
	}

	// clear the current scenario
	Concurrit::set_current_scenario(NULL);
	safe_assert(Scenario::Current() == NULL);

}

/********************************************************************************/

void Scenario::OnSignal(int signal_number) {

	if(BETWEEN(TEST_SETUP, test_status_, TEST_TEARDOWN)) {
		psignal(signal_number, "Signal caught! Finishing.");
		safe_assert(Scenario::Current() == this);
		Finish(new SignalResult(signal_number));
	}
}

/********************************************************************************/

void Scenario::SaveSearchInfo() {
	if(Config::SaveDotGraphToFile != NULL) {
		std::cerr << "Saving dot file of the execution graph to: " << Config::SaveDotGraphToFile << std::endl;
		exec_tree_.SaveDotGraph(Config::SaveDotGraphToFile);
	}

	// close trace file
	if(Config::SaveExecutionTraceToFile) {
		if(trace_file_ != NULL) {
			my_fclose(trace_file_, EXIT_ON_FAIL);
			trace_file_ = NULL;
		}
	}

	// save execution tree schedule to file
	static std::string schedule_file_name = InConcurritWorkDir("schedule.txt");
	PersistentSchedule schedule;
	exec_tree_.node_stack()->ComputeExecutionTreeStack(&schedule);
	schedule.Serializable::Store(schedule_file_name.c_str());
}

/********************************************************************************/

//TransferPoint* Scenario::OnYield(SchedulePoint* spoint, Coroutine* target) {
//
//	MYLOG(2) << SC_TITLE << "OnYield starting";
//
//	YieldPoint* point = spoint->AsYield();
//	Coroutine* source = point->source();
//	std::string label = point->label();
//
//	safe_assert(point->IsResolved());
//	safe_assert(point == source->yield_point()->AsYield());
//	safe_assert(source != NULL && source != target);
//
//	CoroutineGroup group = group_;
//	Coroutine* main = group.main();
//
//	bool from_main = point->source() == main;
//
//	bool is_ending = (label == ENDING_LABEL);
//
//	TransferPoint* transfer = NULL;
//
//	// check if we have a matching one
//	if(schedule_->HasCurrent()) {
//		SchedulePoint* current = schedule_->GetCurrent();
//		transfer = current->AsTransfer();
//		if(transfer == NULL) {
//			// this is a choice point
//			safe_assert(current->IsChoice());
//			// sanity check:
//			safe_assert(current->AsChoice()->source() == source);
//			// skip this yield point, since we are expecting a choice point before
//			goto DONE;
//		}
//
//		MYLOG(2) << SC_TITLE << "Processing transfer from the log: " << transfer->ToString();
//
//		ResolvePoint(transfer);
//		safe_assert(transfer->IsResolved());
//
//		// if source of transfer is not the current source of yield then there is a problem, so backtrack
//		if(transfer->yield()->source() != point->source()) {
//			printf("Execution diverged from the recorded schedule. Backtracking...");
//			TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//		}
//
//		target = transfer->target();
//		safe_assert(target != source);
//
//		if(from_main) {
//			MYLOG(2) << SC_TITLE << "From main";
//			safe_assert(target != main);
//			safe_assert(point->label() == MAIN_LABEL);
//
//			if(transfer->yield()->label() != point->label()) {
//				transfer = NULL;
//				goto DONE;
//			}
//
//			// if backtrack point, choose a new target
//			if(target == NULL) {
//				safe_assert(!transfer->done()->empty());
//				// choose the next target
//				target = GetNextEnabled(transfer);
//				if(target == NULL) {
//					// no enable thread found, so backtrack
//					TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//				}
//			}
//			safe_assert(target != NULL);
//			transfer->set_target(target);
//		} else {
//			MYLOG(2) << SC_TITLE << "From non-main";
//			safe_assert(target != NULL);
//			CHECK(target == main); // this holds for now but, may be relaxed later
//			safe_assert(point->label() != MAIN_LABEL);
//
//			if(transfer->yield()->label() != point->label()) {
//				transfer = NULL;
//				goto DONE;
//			}
//
//			if(!CheckUntil(transfer)) {
//				TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//			}
//		}
//
//		safe_assert(target != NULL);
//		safe_assert(transfer != NULL);
//
//		// check if the target is enabled
//		StatusType status = target->status();
//		if(status < ENABLED && BLOCKED <= status) {
//			TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//		}
//
//		MYLOG(2) << SC_TITLE << "Consuming transfer.";
//		if(!transfer->ConsumeOnce()) {
//			// do not yield yet if not consumed all
//			transfer = NULL;
//			goto DONE;
//		}
//
//		// here we delete and reset the yield point of transfer
//		// this helps to get a yield point with non-null loc and access
//		delete transfer->yield();
//		transfer->set_yield(point);
//
//	//********************************************
//
//	} else {
//		// no more points in the schedule, so create the next new one
//
//		MYLOG(2) << SC_TITLE << "Generating a new transfer";
//
//		if(from_main) {
//			if(target == NULL) {
//				target = GetNextEnabled();
//				if(target == NULL) {
//					// no enabled thread found, so backtrack
//					TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//				}
//			}
//			safe_assert(target != main);
//		} else {
//			safe_assert(target == main);
//
//			if(!CheckUntil(point)) {
//				// else, choose not to continue, so do not yield
//				transfer = NULL;
//				goto DONE;
//			}
//		}
//		if(point->IsTransfer()) {
//			transfer = point->AsTransfer();
//		} else {
//			transfer = new TransferPoint(point, target);
//			// consume all (because this transfer is made from a yield point which is already consumed)
//			transfer->ConsumeAll();
//			// resolve it
//			ResolvePoint(transfer);
//		}
//		safe_assert(transfer->IsResolved());
//		schedule_->AddCurrent(transfer, false); // do not move current yet
//
//	}
//	//********************************************
//
//DONE:
//	// after this point, transfer does not change from non-null to null
//
//	MYLOG(2) << "DONE. Scenario::OnYield chose " << ((transfer != NULL) ? transfer->ToString() : "NULL");
//	if(transfer == NULL) { // no transfer, this may be because the current point is a choice point
//		// if yield of source is a transfer point, then we should not be here
//		safe_assert(!source->yield_point()->IsTransfer());
//
//		// if the ending yield and transfer is still NULL, then there is a problem, so we should backtrack
//		if(is_ending) {
//			TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//		}
//
//		// update schedule to add this yield point
//		if(schedule_->GetCurrent() != point) {
//			MYLOG(2) << SC_TITLE << "Adding yield point " << point->ToString() << " to schedule";
//			schedule_->AddCurrent(point, true);
//		}
//
//	} else { // will take the transfer
//		// transfer is to be taken, so consume the current one
//		safe_assert(schedule_->GetCurrent() == transfer);
//		schedule_->ConsumeCurrent(); // shift the current pointer
//		source->set_yield_point(transfer); // will override point
//
//		safe_assert(transfer->target() != NULL);
//
//		if(!from_main) {
//			// clear the until condition and except set if we are transferring
//			transfer_criteria_.Reset();
//		}
//
//		// target should have already been set.
//		target = transfer->target();
//		safe_assert(target != NULL);
//
//		// set next and previous pointers here
//		SchedulePoint* target_point = target->yield_point();
//		if(target_point != NULL) {
//			YieldPoint* target_yield = target_point->AsYield();
//			safe_assert(target_point->IsTransfer());
//			transfer->set_next(target_yield);
//			target_yield->set_prev(transfer);
//		}
//
//		//-----------------------------------------
//		// updating transfer for DPOR search
//		// we do this only when transfer is the last one in the schedule, means that we have just chosen a new target
//		if(schedule_->GetLast() == transfer) {
//			// TODO(elmas): optimize: do not set all the fields for all kinds of transfers
//
//			// update enabled coroutines
//			CoroutinePtrSet enabled = group_.GetEnabledSet();
//			transfer->set_enabled(enabled);
//
//			// if this is the first time of transfer, add target to backtrack
//			if(transfer->is_first_transition()) {
//				safe_assert(transfer->done()->empty());
//				safe_assert(transfer->backtrack()->empty());
//				transfer->backtrack()->insert(target);
//			}
//
//			// update done set (do this at the end)
//			transfer->done()->insert(target);
//		}
//		// sanity checks
//		safe_assert(!transfer->is_first_transition());
//		// make sure that target is already in done
//		safe_assert(transfer->done()->find(target) != transfer->done()->end());
//	}
//
//	// ensure that if the yield label is "ending", we are transferring (transfer must not be NULL)
//	safe_assert(point->label() != ENDING_LABEL || transfer != NULL);
//
//	return transfer;
//}

/********************************************************************************/

//// Note: this should not update transfer, as we may not have decided on choosing transfer yet
//Coroutine* Scenario::GetNextEnabled(TransferPoint* transfer /*= NULL*/) {
//	CoroutinePtrSet except_targets;
//	CoroutinePtrSet* backtrack = NULL;
//	if(transfer != NULL) {
//		// if first transition, then we choose a random next target
//		// otherwise, we choose a target from backtrack
//		if(!transfer->is_first_transition()) {
//			CoroutinePtrSet* done = transfer->done();
//			safe_assert(!done->empty());
//			except_targets.insert(done->begin(), done->end());
//
//			if(dpor_enabled_) {
//				// if not the first transition, we use backtrack set,
//				// otherwise we chose a random next target
//				backtrack = transfer->backtrack();
//			}
//		}
//	}
//	if(!transfer_criteria_.except()->empty()) {
//		except_targets.insert(transfer_criteria_.except()->begin(), transfer_criteria_.except()->end());
//	}
//	return group_.GetNextEnabled((except_targets.empty() ? NULL : &except_targets), backtrack);
//}

/********************************************************************************/

//SchedulePoint* Scenario::TransferStar(SourceLocation* loc) {
//	return this->Transfer(NULL, loc);
//}

/********************************************************************************/

//// when this begins, we have a particular target to transfer
//// all nondeterminism about transfer is resolved in transfer star
//SchedulePoint* Scenario::Transfer(Coroutine* target, SourceLocation* loc) {
//	MYLOG(2) << SC_TITLE << "Transfer request to " << ((target != NULL) ? to_string(target->tid()) : "star");
//
//	CoroutineGroup group = group_;
//
//	// get current coroutine
//	Coroutine* current = Coroutine::Current();
//	// sanity checks
//	safe_assert(current != NULL);
//
//	if(target != NULL) {
//		safe_assert(group.GetMember(target->tid()) == target);
//
//		// backtrack if target is not enabled
//		if(target->status() != ENABLED) {
//			TRIGGER_BACKTRACK(SPEC_UNSATISFIED);
//		}
//	}
//
//	// this coroutine is main, get the main coroutine from our group
//	Coroutine* main = group.main();
//
//	// check if this is not the main thread (main is not allowed to yield)
//	CHECK(current == main) << "Transfer must be called from the main thread!";
//	CHECK(target != main) << "Transfer must not be targeted to main!";
//
//	std::string main_label(MAIN_LABEL);
//	return do_yield(&group_, main, target, main_label, loc, NULL);
//}

/********************************************************************************/

bool Scenario::AllEnded() {
	return group_.IsAllEnded();
}

/********************************************************************************/

//Scenario* Scenario::UntilEnd() {
//	transfer_criteria_.UntilEnd();
//	return this;
//}

/********************************************************************************/

//Scenario* Scenario::UntilFirst() {
//	transfer_criteria_.UntilFirst();
//	return this;
//}

/********************************************************************************/

//Scenario* Scenario::UntilStar() {
//	transfer_criteria_.UntilStar();
//	return this;
//}

/********************************************************************************/

//Scenario* Scenario::Until(const char* label) {
//	transfer_criteria_.Until(label);
//	return this;
//}

/********************************************************************************/

//Scenario* Scenario::Until(UntilCondition* cond) {
//	transfer_criteria_.Until(cond);
//	return this;
//}

/********************************************************************************/

//Scenario* Scenario::Until(const std::string& label) {
//	transfer_criteria_.Until(label);
//	return this;
//}

/********************************************************************************/

//bool Scenario::CheckUntil(SchedulePoint* point) {
//	safe_assert(transfer_criteria_.until() != NULL);
//	return transfer_criteria_.until()->Check(point);
//}

/********************************************************************************/

//Scenario* Scenario::Except(Coroutine* t) {
//
//	transfer_criteria_.Except(t);
//
//	return this;
//}

/********************************************************************************/

//Scenario* Scenario::Except(THREADID tid) {
//	Coroutine* t = group_.GetMember(tid);
//	safe_assert(t != NULL);
//	return Except(t);
//}

/********************************************************************************/

//void Scenario::OnAccess(Coroutine* current, SharedAccess* access) {
//	safe_assert(access != NULL);
//	safe_assert(access == current->yield_point()->AsYield()->access()); // access must be the last access of current
//
//	MYLOG(2) << SC_TITLE << "Access by " << current->tid() << ": " << access->ToString();
//
//	// notify vc tracker
//	vcTracker_.OnAccess(current->yield_point(), schedule_->coverage());
//}

/********************************************************************************/

//void Scenario::UpdateBacktrackSets() {
//	// iterate over all coroutines in the group
//	for(MembersMap::iterator itr = group_.members()->begin(); itr != group_.members()->end(); ++itr) {
//		Coroutine* co = itr->second;
//		if(co->status() < ENDED) {
//			// handle its next access
//			SchedulePoint* point = co->yield_point();
//			if(point != NULL) {
//				vcTracker_.UpdateBacktrackSets(point);
//			}
//		}
//	}
//}

/********************************************************************************/

//SchedulePoint* yield(const char* label, SourceLocation* loc /*=NULL*/, SharedAccess* access /*=NULL*/, bool force /*=false*/) {
//
//	// give warning if access is null
//	if(access == NULL && strcmp(safe_notnull(label), ENDING_LABEL) != 0) {
//		MYLOG(2) << "YIELD without a shared variable access.";
//		if(loc != NULL) {
//			MYLOG(2) << "Related location: " << loc->ToString();
//		}
//	}
//
//	// get current coroutine
//	Coroutine* current = Coroutine::Current();
//	safe_assert(current != NULL);
//
//	CoroutineGroup* group = current->group();
//	safe_assert(group != NULL);
//
//	Coroutine* main = group->main();
//	safe_assert(main != current);
//
//	std::string strlabel = std::string(label);
//
//	Scenario* scenario = group->scenario();
//	safe_assert(scenario != NULL);
//
//	// if forced yield, put an until condition for this
//	if(force) {
//		// not the ending label
//		safe_assert(strcmp(label, ENDING_LABEL) != 0);
//		// TODO(elmas): check if the current until condition is consistent with forced yield
//		safe_assert(!current->IsMain());
//		scenario->UntilFirst(); // choose the first yield
//	}
//
//	return scenario->do_yield(group, current, main, strlabel, loc, access);
//
//}

/********************************************************************************/

//void Scenario::ExhaustiveSearch() {
////	safe_assert(group_.current() == group_.main());
//
//	TEST_FORALL();
//
//	UNTIL_ALL_END {
//		UNTIL_STAR()->TRANSFER_STAR();
//	}
//}
//
//void Scenario::ContextBoundedExhaustiveSearch(int C) {
////	safe_assert(group_.current() == group_.main());
//	CHECK(C > 0) << "Context bound must be > 0";
//
//	TEST_FORALL();
//
//	Coroutine* prev = NULL;
//	UNTIL_ALL_END {
//		SchedulePoint* point = UNTIL_STAR()->EXCEPT(prev)->TRANSFER_STAR();
//
//		safe_assert(point->IsTransfer());
//		safe_assert(point->AsTransfer()->target() == group_.main());
//
//		prev = point->AsYield()->source();
//		if(C > 0 && --C == 0) break;
//	}
//
//	FINISH_ALL();
//}
//
//void Scenario::NDSeqSearch() {
////	safe_assert(group_.current() == group_.main());
//
//	TEST_FORALL();
//
//	UNTIL_ALL_END {
//		FINISH_STAR();
//	}
//}

/********************************************************************************/

//void Scenario::RunSavedSchedule(const char* filename) {
//	Schedule schedule(filename, true); // load it
//	schedule_->extend_points(&schedule);
//}

/********************************************************************************/

//void BeginStrand(const char* name) {
//	safe_assert(name != NULL);
//	// printf("Resuming thread %s...\n", name);
//}

/********************************************************************************/

//SchedulePoint* Scenario::Yield(Scenario* scenario, CoroutineGroup* group, Coroutine* current, Coroutine* target, std::string& label, SourceLocation* loc, SharedAccess* access) {
//	MYLOG(2) << "Scenario::Yield (default implementation)";
//
//	CHECK(scenario == this);
//
//	// sanity checks
//	safe_assert(current != NULL);
//	safe_assert(group != NULL);
//	safe_assert(scenario == group->scenario());
//	safe_assert(group == scenario->group());
//
//	MYLOG(2) << "Yield request from " << current->tid();
//
//	// update backtrack set before taking the transition
//	this->UpdateBacktrackSets();
//
//	// generate or update the current yield point of current
//	SchedulePoint* point = current->OnYield(target, label, loc, access);
//
//	// make the next decision
//	TransferPoint* transfer = this->OnYield(point, target);
//
//	SchedulePoint* target_point = NULL;
//
//	// point != NULL means go to the target
//	// point == NULL means do not yield
//	if(transfer != NULL) {
//		safe_assert(transfer->IsResolved());
//		safe_assert(transfer->AsYield()->source() == current);
//		safe_assert(transfer->AsYield()->label() == label);
//		safe_assert(transfer->rem_count() == 0);
//
//		Coroutine* target = transfer->target();
//		safe_assert(transfer->target() != NULL);
//
//		// do the transfer
//		current->Transfer(target, MSG_TRANSFER);
//
//		// return yield point of the target
//		target_point = target->yield_point();
//	}
//
//	//----------------------------------------
//	// after transfer returns back to current
//
//	return target_point; // means did not transfer to another thread
//}

/********************************************************************************/

// TODO(elmas): implement
void Scenario::UpdateAlternateLocations(Coroutine* current) {
//	if(!Config::TrackAlternatePaths) return;
//
//	// evaluate other nodes in current_nodes_
//	std::vector<ChildLoc>* current_nodes = exec_tree_.current_nodes();
//	// do not touch the first one, which is the current one
//	std::vector<ChildLoc>::iterator itr = current_nodes->begin();
//	for(; itr < current_nodes->end();) {
//		ChildLoc loc = *itr;
//		ExecutionTree* parent = loc.parent();
//		safe_assert(parent != NULL);
//		// this must be a transition node
//		TransitionNode* trans = ASINSTANCEOF(parent, TransitionNode*);
//		if(trans != NULL) {
//			ChildLoc newloc;
//			TPVALUE tval = pre_state ?
//							EvalPreState(current, trans, &newloc) :
//							EvalPostState(current, trans, &newloc);
//			// if pre-state, then tval is false or unknown
//			safe_assert(!pre_state || tval == TPFALSE || tval == TPUNKNOWN);
//			if(tval == TPTRUE) {
//				// update iterator and continue
//				safe_assert(newloc.parent() != NULL);
//				*itr = newloc;
//				++itr;
//			} else if(tval == TPFALSE) {
//				// remove this
//				itr = current_nodes->erase(itr);
//			} else {
//				// will check in the post state
//				++itr;
//			}
//		} else {
//			// unsupported node, delete it
//			itr = current_nodes->erase(itr);
//		}
//	}
//	// populate reachable nodes after the transition
//	if(!pre_state) {
//		// evaluate other nodes in current_nodes_
//		for(int i = 0, sz = current_nodes->size(); i < sz; ++i) {
//			ChildLoc loc = (*current_nodes)[i];
//			ExecutionTree* parent = loc.parent();
//			safe_assert(parent != NULL); // TODO(elmas): this can be NULL?
//			// populate alternate paths
//			parent->PopulateLocations(loc.child_index(), current_nodes);
//		}
//	}
}

/********************************************************************************/

void Scenario::EvalSelectThread(Coroutine* current, SelectThreadNode* node, int& child_index, bool& take) {
	safe_assert(current != NULL && node != NULL);
	safe_assert(child_index == -1 && take == false);

	// set current thread id
	AuxState::Tid->set_thread(current);

	// if select is covered, we should not be reaching here
	safe_assert(!node->covered());

	// check if this thread has been covered
	THREADID tid = current->tid();
	safe_assert(tid >= 0);

	ForallThreadNode* forall = ASINSTANCEOF(node, ForallThreadNode*);
	if(forall != NULL) {
		MYLOG(2) << "Evaluating forall-thread node";

		// first check the stack
		ChildLoc loc_in_stack = exec_tree_.GetNextNodeInStack();
		int idx_in_stack = loc_in_stack.child_index();
		safe_assert((idx_in_stack < 0 && loc_in_stack.parent() == NULL) || loc_in_stack.parent() == node);
//		if(idx_in_stack == -1 || (forall->var(idx_in_stack)->thread()->tid() == tid)) {

//			ExecutionTree* child = forall->child_by_tid(tid);
//			if(child == NULL || !child->covered()) {
//				// set the selected thread to current
//				child_index = forall->set_selected_thread(current);
//				safe_assert(child_index >= 0);
//
//				// check condition
//				if(!forall->CanSelectThread(child_index)) {
//					child_index = -1;
//				}
//			}
			child_index = forall->CheckAndSelectThread(current, idx_in_stack);
//		}

	} else { //=======================================================
		ExistsThreadNode* exists = ASINSTANCEOF(node, ExistsThreadNode*);
		if(exists != NULL) {
			MYLOG(2) << "Evaluating exists-thread node";

			// first check the stack
			ChildLoc loc_in_stack = exec_tree_.GetNextNodeInStack();
			int idx_in_stack = loc_in_stack.child_index();
			safe_assert((idx_in_stack < 0 && loc_in_stack.parent() == NULL) || loc_in_stack.parent() == node);
//			if(idx_in_stack == -1 || exists->lvar()->tid() == tid) {

//				std::set<THREADID>* tids = exists->covered_tids();
//				if(tids->find(tid) == tids->end()) {
//
//					child_index = exists->set_selected_thread(current);
//					safe_assert(child_index >= 0);
//
//					// check condition
//					if(!exists->CanSelectThread(child_index)) {
//						child_index = exists->clear_selected_thread();
//					}
//				}
				child_index = exists->CheckAndSelectThread(current, idx_in_stack);
//			}
		} else {
			safe_fail("Unknown select thread node!");
		}
	}

}

/********************************************************************************/

void Scenario::EvalTransition(Coroutine* current, TransitionNode* node, int& child_index, bool& take) {
	safe_assert(current != NULL && node != NULL);
	safe_assert(child_index == -1 && take == false);

	// set current thread id
	AuxState::Tid->set_thread(current);

	//==========================================//

	// first check assertions
	TransitionAssertionsPtr assertions = trans_assertions_;
	if(assertions != NULL && !assertions->EvalState(current)) {
		TRIGGER_ASSERTION_VIOLATION(assertions->ToString().c_str(), "", "", 0);
	}

	//==========================================//

	// then check constraints
	TransitionConstraintsPtr constraints = trans_constraints_;
	if(constraints != NULL && !constraints->EvalState(current)) {
		return;
	}

	//==========================================//

	RunThroughNode* runthrough = ASINSTANCEOF(node, RunThroughNode*);
	if(runthrough != NULL) {

		// now evaluate the actual predicate
		TransitionPredicatePtr pred = runthrough->pred();
		safe_assert(pred != NULL);

		if(pred->EvalState(current)) {
			MYLOG(2) << "Will consume the current run-through";
			child_index = 0;
		}
		take = true;

	} else {

		RunUntilNode* rununtil = ASINSTANCEOF(node, RunUntilNode*);
		if(rununtil != NULL) {
			// now evaluate the actual predicate
			TransitionPredicatePtr pred = rununtil->pred();
			safe_assert(pred != NULL);

			if(pred->EvalState(current)) {
				MYLOG(2) << "Will consume the current run-until";
				child_index = 0;
				take = false;
			} else {
				take = true;
			}

		} else {
			safe_fail("Unknown transition node!");
		}
	}

	//==========================================//

	if(take) {
		node->OnTaken(current, 0);
	}
}

/********************************************************************************/

//TPVALUE Scenario::EvalPostState(Coroutine* current, TransitionNode* node, ChildLoc* newnode) {
//
//	// value determining what to with the transition
//	// TPTRUE: consume the transition and continue as normal
//	// TPFALSE: trigger backtrack, as the node is not satisfied by the current transition
//	// TPUNKNOWN: will keep the node, but we put it back to atomic_ref to take it later
//	// TPINVALID: cannot occur
//	TPVALUE tval = TPTRUE;
//
//	// set current thread id
//	AuxState::Tid->set_thread(current);
//
//	TransitionConstraintsPtr constraints = node->constraints();
//
////	SingleTransitionNode* trans = ASINSTANCEOF(node, SingleTransitionNode*);
////	if(trans != NULL) {
////
////		bool ret = constraints != NULL ? constraints->EvalPostState(current) : TPTRUE;
////		tval = (ret ? TPTRUE : TPFALSE);
////		if(ret) {
////
////			// now evaluate the actual predicate
////			TransitionPredicatePtr pred = trans->pred();
////			safe_assert(pred != NULL);
////			ret = pred->EvalPostState(current);
////
////			tval = (ret ? TPTRUE : TPFALSE);
////			if(tval == TPTRUE) {
////				MYLOG(2) << "Will consume the current transition";
////				*newnode = {trans, 0}; // newnode is the next of node
////			}
////		}
////	} else { //=============================================================
//		TransferUntilNode* truntil = ASINSTANCEOF(node, TransferUntilNode*);
//		if(truntil != NULL) {
//
//			// first check the assertion
//			TransitionPredicatePtr assertion = truntil->assertion();
//			if(assertion != NULL && assertion != TransitionPredicate::True()) {
//				// do check assertion
//				if(!assertion->EvalPostState(current)) {
//					// trigger assertion violation
//					TRIGGER_ASSERTION_VIOLATION(assertion->ToString().c_str(), "", "", 0);
//				}
//			}
//
//			bool ret = constraints != NULL ? constraints->EvalPostState(current) : TPTRUE;
//			tval = (ret ? TPTRUE : TPFALSE);
//			if(ret) {
//
//				// now evaluate the actual predicate
//				TransitionPredicatePtr pred = truntil->pred();
//				safe_assert(pred != NULL);
//				ret = pred->EvalPostState(current);
//
//				tval = (ret ? TPTRUE : TPFALSE);
//
//				if(tval == TPFALSE) {
//					// continue holding the node
//					// the following causes holding the node in the switch below
//					tval = TPUNKNOWN;
//				}
//				else if(tval == TPTRUE) {
//					MYLOG(2) << "Will consume the current transfer-until";
//					*newnode = {truntil, 0}; // newnode is the next of node
//				}
//			}
//
//		} else {
//			TransferUnlessNode* trunless = ASINSTANCEOF(node, TransferUnlessNode*);
//			if(trunless != NULL) {
//
//				// first check the assertion
//				TransitionPredicatePtr assertion = trunless->assertion();
//				if(assertion != NULL && assertion != TransitionPredicate::True()) {
//					// do check assertion
//					if(!assertion->EvalPostState(current)) {
//						// trigger assertion violation
//						TRIGGER_ASSERTION_VIOLATION(assertion->ToString().c_str(), "", "", 0);
//					}
//				}
//
//				bool ret = constraints != NULL ? constraints->EvalPostState(current) : TPTRUE;
//				tval = (ret ? TPTRUE : TPFALSE);
//				if(ret) {
//
//					// now evaluate the actual predicate
//					TransitionPredicatePtr pred = trunless->pred();
//					safe_assert(pred != NULL);
//					ret = pred->EvalPostState(current);
//
//					tval = (ret ? TPTRUE : TPFALSE);
//
//					if(tval == TPFALSE) {
//						// continue holding the node
//						// the following causes holding the node in the switch below
//						tval = TPUNKNOWN;
//					}
//					else if(tval == TPTRUE) {
//						MYLOG(2) << "tval should not ve TPTRUE for transferunless in the post state";
//						// tval should not be TPTRUE, so we make it false, and this causes a backtrack
//						tval = TPFALSE;
//					}
//				}
//
//			} else {
//				safe_assert(false); // unknown node type
//			}
//		}
////	}
//
//	safe_assert(tval == TPFALSE || tval == TPTRUE || tval == TPUNKNOWN);
//
//	// we do things to consume the transition here
//	if(tval == TPTRUE || tval == TPUNKNOWN) {
//		node->OnTaken(current, tval == TPUNKNOWN ? 0 : newnode->child_index());
//	}
//
//	if(tval == TPTRUE) {
//		node->OnConsumed(current, tval == TPUNKNOWN ? 0 : newnode->child_index());
//	}
//
//	return tval;
//}

/********************************************************************************/

//// program state should be updated before this point
//void Scenario::BeforeControlledTransition(Coroutine* current) {
//	CHECK(!Config::RunUncontrolled) << "Hit a controlled transition in an uncontrolled run!";
//
//	// make thread blocked
//	if(current->status() != ENABLED) return;
//
//	current->set_status(BLOCKED);
//
//	MYLOG(2) << "Before controlled transition by " << current->tid();
//
//	// to continue waiting or exit?
//	bool done = true;
//
//	//=================================================================
//
//	ExecutionTree* prev_unsat_node = NULL; // previous node which was not satisfied
//
//	do { // we loop until either we get hold of and process the current node in the execution tree
//
//		// new element to be used when the current node is consumed
//		ChildLoc newnode = {NULL, -1};
//
//		// value determining what to with the transition
//		// TPTRUE: cannot occur, we leave the decision to after the transition (TPTRUE result is interpreted as TPUNKNOWN)
//		// TPFALSE: release the node back
//		// TPUNKNOWN: keep the node and decide after the transition
//		// TPINVALID: cannot occur
//		TPVALUE tval =  TPTRUE;
//
//		if(!done) {
//			Thread::Yield(true);
//		} else {
//			done = false;
//		}
//
//		// we always start with NULL current node
////		safe_assert(current->current_node() == NULL);
//
//		//=================================================================
//		MYLOG(2) << "Acquiring Ref with EXIT_ON_FULL";
//		// get the node
//		ExecutionTree* node = exec_tree_.AcquireRef(EXIT_ON_FULL);
//
//		// if end_node, exit immediatelly
//		if(exec_tree_.IS_ENDNODE(node)) {
//			MYLOG(2) << "Detected end node, exiting handling code";
//			// uncontrolled mode, but the mode is set by main, so just ignore the rest
//			// no need to release node, because AcquireRef does not overwrite end nodes.
////			current->set_current_node(node); // this is to inform AfterControlledTransition for the ending
//			return;
//		}
//
//		safe_assert(node != NULL);
//		safe_assert(!exec_tree_.IS_EMPTY(node) && !exec_tree_.IS_LOCKNODE(node));
//
//		// if previously unsatisfied node, then retry
//		if(node == prev_unsat_node) {
//
//			MYLOG(2) << "Detected same unsatisfied node, will retry";
//			tval = TPFALSE; // indicates that node was unsatisfied (will retry for another node)
//
//		} else {
//			//=================================================================
//			MYLOG(2) << "Checking execution tree node type";
//			// get the node
//			TransitionNode* trans = ASINSTANCEOF(node, TransitionNode*);
//			if(trans != NULL) {
//				MYLOG(2) << "Evaluating transition node";
//
//				tval = EvalPreState(current, trans, &newnode);
//
//				done = (tval == TPUNKNOWN);
//
////			} else { //=======================================================
////				TransferUntilNode* truntil = ASINSTANCEOF(node, TransferUntilNode*);
////				if(truntil != NULL) {
////					MYLOG(2) << "Evaluating transfer-until predicate";
////
////					tval = EvalPreState(current, truntil, &newnode);
////
////					done = (tval == TPUNKNOWN);
//
//				} else { //=======================================================
//					SelectThreadNode* select = ASINSTANCEOF(node, SelectThreadNode*);
//					if(select != NULL) {
//						MYLOG(2) << "Evaluating select-thread node";
//
//						tval = EvalSelectThread(current, select, &newnode);
//
//						// we are not done yet, so leave done as false
//						safe_assert(!done);
//
//					} else { //=======================================================
//						// TODO(elmas): others are not supported yet
//						safe_assert(false);
//					}
////				}
//			}
//		}
//
//SWITCH:
//
//		//=================================================================
//		MYLOG(2) << "Switching on three-valued variable";
//		// take action depending on tval
//		switch(tval) {
//		case TPTRUE: // consume the transition
//			MYLOG(2) << "Consuming transition";
//			// in this case, we insert a new node to the path represented by newnode
//			safe_assert(!newnode.empty());
//			safe_assert(exec_tree_.IS_SELECTTHREADNODE(node) || INSTANCEOF(node, TransferUnlessNode*));
//			exec_tree_.ReleaseRef(newnode);
//			// do not exit method
//			safe_assert(!done);
//			break;
//
//		case TPFALSE: // release the transition and wait
//			MYLOG(2) << "Releasing transition back";
//			exec_tree_.ReleaseRef(node);
//
//			prev_unsat_node = node; // update previously unsatisfied node
//			Thread::Yield(true);
//			// do not exit method
//			safe_assert(!done);
//			break;
//
//		case TPUNKNOWN: // take the transition and decide after the transition
//			MYLOG(2) << "Unknown, to be decided after the transition";
////			current->set_current_node(node);
//			UpdateAlternateLocations(current, true);
//			safe_assert(done);
//			break;
//
//		default:
//			fprintf(stderr, "Invalid TPVALUE: %d\n", tval);
//			bool InvalidTPValue = false;
//			safe_assert(InvalidTPValue);
//			break;
//		}
//
//	//=================================================================
//
//	} while(!done);
//}

/********************************************************************************/

// program state should be updated before this point
void Scenario::OnControlledTransition(Coroutine* current) {
	CHECK(!Config::RunUncontrolled) << "Hit a controlled transition in an uncontrolled run!";

	// make thread blocked
	if(current->status() != ENABLED) return;

	MYLOG(2) << "Before controlled transition by " << current->tid();

	current->StartControlledTransition();

	// to continue waiting or exit?
	bool take = false;

	//=================================================================

	ExecutionTree* prev_unsat_node = NULL; // previous node which was not satisfied

	for(;;) { // we loop until either we get hold of and process the current node in the execution tree

		// to consume the current node or not?
		int child_index = -1;

		// we always start with NULL current node
//		safe_assert(current->current_node() == NULL);

		//=================================================================
		MYLOG(3) << "Acquiring Ref with EXIT_ON_FULL";
		// get the node
		ExecutionTree* node = exec_tree_.AcquireRef(EXIT_ON_FULL);

		// if end_node, exit immediatelly
		if(exec_tree_.IS_ENDNODE(node)) {
			MYLOG(2) << "Detected end node, exiting handling code";
			// uncontrolled mode, but the mode is set by main, so just ignore the rest
			// no need to release node, because AcquireRef does not overwrite end nodes.
//			current->set_current_node(node); // this is to inform AfterControlledTransition for the ending
			break;
		}

		safe_assert(node != NULL);
		safe_assert(!exec_tree_.IS_EMPTY(node) && !exec_tree_.IS_LOCKNODE(node));

		// if previously unsatisfied node, then retry
		if(node != prev_unsat_node) {

			MYLOG(2) << "Checking execution tree node type";

			//=======================================================

			TransitionNode* trans = ASINSTANCEOF(node, TransitionNode*);
			if(trans != NULL) {
				MYLOG(2) << "Evaluating transition node";

				EvalTransition(current, trans, child_index, take);

			} else {

				//=======================================================

				SelectThreadNode* select = ASINSTANCEOF(node, SelectThreadNode*);
				if(select != NULL) {
					MYLOG(2) << "Evaluating select-thread node";

					EvalSelectThread(current, select, child_index, take);

					// we are not done yet, so leave done as false
					safe_assert(!take);

				} else { //=======================================================
					// TODO(elmas): others are not supported yet
					safe_assert(false);
				}
			}
		}

		//=================================================================

		if(child_index >= 0) {
			MYLOG(2) << "Consuming transition";
			safe_assert(node != NULL);
			// in this case, we insert a new node to the path represented by newnode
			node->OnConsumed(current, child_index);
			exec_tree_.ReleaseRef(node, child_index);

		} else {
			MYLOG(3) << "Releasing transition back";
			exec_tree_.ReleaseRef(node);
		}

		//=================================================================

		if(take) {
			UpdateAlternateLocations(current);
			break;
		} else {
			prev_unsat_node = node; // update previously unsatisfied node
			Thread::Yield(true);
		}
	} // end loop


	//=================================================================

	MYLOG(2) << "After controlled transition by " << current->tid();

	current->FinishControlledTransition();
}

/********************************************************************************/

//void Scenario::AfterControlledTransition(Coroutine* current) {
//	CHECK(!Config::RunUncontrolled) << "Hit a controlled transition in an uncontrolled run!";
//
//	if(current->status() != BLOCKED) return;
//
//	MYLOG(2) << "After controlled transition by " << current->tid();
//
//	// value determining what to with the transition
//	// TPTRUE: consume the transition and continue as normal
//	// TPFALSE: trigger backtrack, as the node is not satisfied by the current transition
//	// TPUNKNOWN: will keep the node, but we put it back to atomic_ref to take it later
//	// TPINVALID: cannot occur
//	TPVALUE tval =  TPTRUE;
//
//	//=================================================================
//	// if we have a current node, then check it
//	ExecutionTree* node = current->current_node();
//	safe_assert(node != NULL) {
//
//		// if end_node, exit immediatelly
//		if(exec_tree_.IS_ENDNODE(node)) {
//			MYLOG(2) << "Detected end node, exiting handling code";
//			current->FinishControlledTransition();
//			return;
//		}
//
//		// new element to be used when the current node is consumed
//		ChildLoc newnode = {NULL, -1};
//
//		MYLOG(2) << "Checking execution tree node type";
//		safe_assert(!ASINSTANCEOF(node, SelectThreadNode*));
//
//		TransitionNode* trans = ASINSTANCEOF(node, TransitionNode*);
//		if(trans != NULL) {
//			MYLOG(2) << "Evaluating transition node";
//
//			tval = EvalPostState(current, trans, &newnode);
//
////		} else {//=======================================================
////			safe_assert(!ASINSTANCEOF(node, SelectThreadNode*));
////			MYLOG(2) << "Evaluating transfer-until predicate";
////			TransferUntilNode* truntil = ASINSTANCEOF(node, TransferUntilNode*);
////			if(truntil != NULL) {
////
////				tval = EvalPostState(current, truntil, &newnode);
//
//			} else {
//				// TODO(elmas): others are not supported yet
//				safe_assert(false);
//			}
////		}
//
//		//=================================================================
//		MYLOG(2) << "Switching on three-valued variable";
//		safe_assert(exec_tree_.IS_TRANSNODE(node));
//		// take action depending on tval
//		switch(tval) {
//		case TPTRUE: // consume the transition if node is not null, otherwise, there was no node to handle
//			MYLOG(2) << "Consuming transition";
//			safe_assert(node != NULL);
//			// in this case, we insert a new node to the path represented by newnode
//			safe_assert(newnode.parent() != NULL && newnode.child_index() >= 0);
//			UpdateAlternateLocations(current, false);
//			exec_tree_.ReleaseRef(newnode);
//			break;
//
//		case TPFALSE: // trigger backtrack
//			MYLOG(2) << "Transition not satisfied, will release transition back and backtrack";
//			exec_tree_.ReleaseRef(node);
//			// trigger backtrack, causes current execution to end
//			exec_tree_.EndWithBacktrack(current, SPEC_UNSATISFIED, "AfterControlledTransition");
//			break;
//
//		case TPUNKNOWN: // continue holding the transition node
//			MYLOG(2) << "Unknown, with the same transition";
//			safe_assert(exec_tree_.IS_MULTITRANSNODE(node));
//			safe_assert(current->current_node() == node);
//			UpdateAlternateLocations(current, false);
//			// we release the node back (will take it later)
//			exec_tree_.ReleaseRef(node);
//			break;
//
//		default:
//			fprintf(stderr, "Invalid TPVALUE: %d\n", tval);
//			bool InvalidTPValue = false;
//			safe_assert(InvalidTPValue);
//			break;
//		}
//
//	} // end if(node != NULL)
//
//	//=================================================================
//	current->FinishControlledTransition();
//}

/********************************************************************************/

bool Scenario::Backtrack(BacktrackReason reason) {
	MYLOG(2) << "Backtrack for reason: " << reason;

	return !exec_tree_.ROOTNODE()->covered();
}

/********************************************************************************/

bool Scenario::DSLChoice(StaticDSLInfo* static_info, const char* message /*= NULL*/) {
	safe_assert(static_info != NULL);
	if(message != NULL) static_info->set_message(message);

	MYLOG(2) << "Adding DSLChoice";

	//=======================================================

//	if(Config::TrackAlternatePaths && is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->pop();
//		safe_assert(!reploc.empty());
//		safe_assert(BETWEEN(0, reploc.child_index(), 1));
//		ChoiceNode* choice = ASINSTANCEOF(reploc.parent(), ChoiceNode*);
//		safe_assert(choice != NULL);
//
//		// update current node
//		exec_tree_.AddToNodeStack(reploc);
//
//		MYLOG(2) << "Replaying path with choice node.";
//		return (reploc.child_index() == 1);
//	}

	//=======================================================

	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

	ChoiceNode* choice = NULL;
	node = exec_tree_.GetNextNodeInStack().parent();
	if(node != NULL) {
		choice = ASINSTANCEOF(node, ChoiceNode*);
		safe_assert(choice != NULL);
		// recompute coverage, since static info might be updated to set one of the branches covered
		if(choice->ComputeCoverage()) {
			// release lock
			exec_tree_.ReleaseRef(NULL);
			// backtrack
			TRIGGER_BACKTRACK(TREENODE_COVERED);
		}
//		if(choice == NULL || choice->covered()) {
//			safe_assert(choice != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
	} else {
		choice = new ChoiceNode(static_info);
	}

	safe_assert(choice != NULL && !choice->covered());

	// 0: false, 1: true
	ChildLoc loc_in_stack = exec_tree_.GetNextNodeInStack();
	int ret = loc_in_stack.child_index();
	safe_assert(BETWEEN(-1, ret, 1));
	safe_assert((ret < 0 && loc_in_stack.parent() == NULL) || loc_in_stack.parent() == choice);
	if(ret < 0) {
		bool cov_0 = choice->child_covered(0);
		bool cov_1 = choice->child_covered(1);
		safe_assert(!cov_0 || !cov_1);

		if(!cov_0 && !cov_1) {
			if(safe_notnull(ASINSTANCEOF(choice->static_info(), StaticChoiceInfo*))->nondet()) {
				ret = (generate_random_bool() ? 1 : 0);
			} else {
				ret = 1;
			}
		} else {
			ret = !cov_0 ? 0 : 1;
		}
	}

	safe_assert(!choice->child_covered(ret));

	choice->OnSubmitted();
	choice->OnConsumed(Coroutine::Current(), ret);

	exec_tree_.ReleaseRef(choice, ret);

	MYLOG(2) << "DSLChoice returns " << ret;

	return (ret == 1);
}

/********************************************************************************/

bool Scenario::DSLConditional(StaticDSLInfo* static_info, bool value, const char* message /*= NULL*/) {
	safe_assert(static_info != NULL);
	if(message != NULL) static_info->set_message(message);

	MYLOG(2) << "Adding DSLConditional";

	//=======================================================

//	if(Config::TrackAlternatePaths && is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->pop();
//		safe_assert(!reploc.empty());
//		safe_assert(BETWEEN(0, reploc.child_index(), 1));
//		ConditionalNode* choice = ASINSTANCEOF(reploc.parent(), ConditionalNode*);
//		safe_assert(choice != NULL);
//
//		// update current node
//		exec_tree_.AddToNodeStack(reploc);
//
//		MYLOG(2) << "Replaying path with conditional node.";
//		return (reploc.child_index() == 1);
//	}

	//=======================================================

	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

	ConditionalNode* choice = NULL;
	node = exec_tree_.GetNextNodeInStack().parent();
	if(node != NULL) {
		choice = ASINSTANCEOF(node, ConditionalNode*);
		safe_assert(choice != NULL);
		// recompute coverage, since static info might be updated to set one of the branches covered
		if(choice->ComputeCoverage()) {
			// release lock
			exec_tree_.ReleaseRef(NULL);
			// backtrack
			TRIGGER_BACKTRACK(TREENODE_COVERED);
		}
//		if(choice == NULL || choice->covered()) {
//			safe_assert(choice != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
	} else {
		choice = new ConditionalNode(static_info, value);
	}

	safe_assert(choice != NULL && !choice->covered());

	ChildLoc loc_in_stack = exec_tree_.GetNextNodeInStack();
	int child_index = loc_in_stack.child_index();
	safe_assert(child_index == -1 || child_index == 0);
	safe_assert((child_index < 0 && loc_in_stack.parent() == NULL) || loc_in_stack.parent() == choice);

	if(choice->value() != value) {
		// release lock
		exec_tree_.ReleaseRef(NULL);
		Scenario::NotNullCurrent()->counter("Num replay fails").increment();
		TRIGGER_BACKTRACK(REPLAY_FAILS);
	}

	safe_assert(!choice->child_covered(0));

	choice->OnSubmitted();
	choice->OnConsumed(Coroutine::Current(), 0);

	exec_tree_.ReleaseRef(choice, 0);

	MYLOG(2) << "DSLConditional returns " << value;

	return value;
}

/********************************************************************************/

//void Scenario::DSLTransition(const TransitionPredicatePtr& assertion, const TransitionPredicatePtr& pred, const ThreadVarPtr& var /*= ThreadVarPtr()*/, const char* message /*= NULL*/) {
//	MYLOG(2) << "Adding DSLTransition";
//
//	//=======================================================
//
//	if(is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->back(); exec_tree_.replay_path()->pop_back();
//		safe_assert(!reploc.empty());
//		SingleTransitionNode* trans = ASINSTANCEOF(reploc.parent(), SingleTransitionNode*);
//		safe_assert(trans != NULL);
//		trans->Update(assertion, pred, var, trans_constraints_->Clone(), message);
//
//		// update current node
//		exec_tree_.set_current_node(reploc);
//
//		MYLOG(2) << "Replaying path with single transition node.";
//		return;
//	}
//
//	//=======================================================
//
//	if(group_.IsAllEnded()) {
//		TRIGGER_BACKTRACK(THREADS_ALLENDED);
//	}
//
//	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
//	safe_assert(node == NULL);
//
//	SingleTransitionNode* trans = NULL;
//	node = exec_tree_.GetLastInPath();
//	if(node != NULL) {
//		safe_assert(exec_tree_.GetLastInPath().check(node));
//		trans = ASINSTANCEOF(node, SingleTransitionNode*);
//		if(trans == NULL || trans->covered()) {
//			safe_assert(trans != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
//		trans->Update(assertion, pred, var, trans_constraints_->Clone(), message);
//	} else {
//		trans = new SingleTransitionNode(assertion, pred, var, trans_constraints_->Clone(), message);
//	}
//
//	safe_assert(!trans->covered());
//
//	// not covered yet
//
//	trans->OnSubmitted();
//
//	// set atomic_ref to point to trans
//	exec_tree_.ReleaseRef(trans);
//
//	//=======================================================
//	// wait for the consumption
//
//	node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
//	safe_assert(node == NULL);
//	exec_tree_.ReleaseRef(NULL);
//
//	MYLOG(2) << "Added DSLTransition.";
//}

/********************************************************************************/

void Scenario::DSLRunThrough(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const char* message /*= NULL*/) {
	DSLRunThrough(static_info, pred, ThreadVarPtr(), message);
}

/********************************************************************************/

void Scenario::DSLRunThrough(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const ThreadVarPtr& var /*= ThreadVarPtr()*/, const char* message /*= NULL*/) {
	safe_assert(static_info != NULL);
	if(message != NULL) static_info->set_message(message);

	MYLOG(2) << "Adding DSLRunThrough";

	//=======================================================

//	if(Config::TrackAlternatePaths && is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->pop();
//		safe_assert(!reploc.empty());
//		safe_assert(BETWEEN(0, reploc.child_index(), 1));
//		TransferUntilNode* trans = ASINSTANCEOF(reploc.parent(), TransferUntilNode*);
//		safe_assert(trans != NULL);
//		trans->Update(pred, var);
//
//		// update current node
//		exec_tree_.AddToNodeStack(reploc);
//
//		MYLOG(2) << "Replaying path with transfer until node.";
//		return;
//	}

	//=======================================================

	if(group_.IsAllEnded()) {
		TRIGGER_BACKTRACK(THREADS_ALLENDED);
	}

	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

	RunThroughNode* trans = NULL;
	node = exec_tree_.GetNextNodeInStack().parent();
	if(node != NULL) {
		trans = ASINSTANCEOF(node, RunThroughNode*);
		safe_assert(trans != NULL && !trans->covered());
//		if(trans == NULL || trans->covered()) {
//			safe_assert(trans != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
		trans->Init(pred, var);
	} else {
		trans = new RunThroughNode(static_info, pred, var);
	}

	safe_assert(trans != NULL && !trans->covered());

	// not covered yet

	trans->OnSubmitted();

	// set atomic_ref to point to trans
	exec_tree_.ReleaseRef(trans);

	//=======================================================
	// wait for the consumption

	node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);
	exec_tree_.ReleaseRef(NULL);

	MYLOG(2) << "Added DSLRunThrough.";
}

/********************************************************************************/

void Scenario::DSLRunUntil(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const char* message /*= NULL*/) {
	DSLRunUntil(static_info, pred, ThreadVarPtr(), message);
}

/********************************************************************************/

void Scenario::DSLRunUntil(StaticDSLInfo* static_info, const TransitionPredicatePtr& pred, const ThreadVarPtr& var /*= ThreadVarPtr()*/, const char* message /*= NULL*/) {
	safe_assert(static_info != NULL);
	if(message != NULL) static_info->set_message(message);

	MYLOG(2) << "Adding DSLRunUntil";

	//=======================================================

//	if(Config::TrackAlternatePaths && is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->pop();
//		safe_assert(!reploc.empty());
//		safe_assert(BETWEEN(0, reploc.child_index(), 1));
//		TransferUnlessNode* trans = ASINSTANCEOF(reploc.parent(), TransferUnlessNode*);
//		safe_assert(trans != NULL);
//		trans->Update(pred, var);
//
//		// update current node
//		exec_tree_.AddToNodeStack(reploc);
//
//		MYLOG(2) << "Replaying path with transfer unless node.";
//		return;
//	}

	//=======================================================

	if(group_.IsAllEnded()) {
		TRIGGER_BACKTRACK(THREADS_ALLENDED);
	}

	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

	RunUntilNode* trans = NULL;
	node = exec_tree_.GetNextNodeInStack().parent();
	if(node != NULL) {
		trans = ASINSTANCEOF(node, RunUntilNode*);
		safe_assert(trans != NULL && !trans->covered());
//		if(trans == NULL || trans->covered()) {
//			safe_assert(trans != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
		trans->Init(pred, var);
	} else {
		trans = new RunUntilNode(static_info, pred, var);
	}

	safe_assert(trans != NULL && !trans->covered());

	// not covered yet

	trans->OnSubmitted();

	// set atomic_ref to point to trans
	exec_tree_.ReleaseRef(trans);

	//=======================================================
	// wait for the consumption

	node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);
	exec_tree_.ReleaseRef(NULL);

	MYLOG(2) << "Added DSLRunUntil.";
}


/********************************************************************************/

ThreadVarPtr Scenario::DSLForallThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const char* message /*= NULL*/) {
	return DSLForallThread(static_info, scope, PTRUE, ANY_THREAD, message);
}

ThreadVarPtr Scenario::DSLForallThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred /*= TransitionPredicatePtr()*/, const char* message /*= NULL*/) {
	return DSLForallThread(static_info, scope, pred, ANY_THREAD, message);
}

ThreadVarPtr Scenario::DSLForallThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred /*= TransitionPredicatePtr()*/, const ThreadExprPtr& texpr /*= ThreadExprPtr()*/, const char* message /*= NULL*/) {
	safe_assert(static_info != NULL);
	if(message != NULL) static_info->set_message(message);

	MYLOG(2) << "Adding DSLForallThread";

	//=======================================================

	ThreadVarPtr var;

//	if(Config::TrackAlternatePaths && is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->pop();
//		safe_assert(reploc.parent() != NULL);
//		ForallThreadNode* select = ASINSTANCEOF(reploc.parent(), ForallThreadNode*);
//		safe_assert(select != NULL);
//		safe_assert(BETWEEN(0, reploc.child_index(), int(select->children()->size())-1));
//
//		int child_index = reploc.child_index();
//		// if index is -1, then we should submit this select thread
//		if(child_index >= 0) {
//			// re-select the thread to update the associated thread variable
//			select->Update(pred);
//			select->set_selected_thread(child_index);
//			var = select->var(child_index);
//			safe_assert(var != NULL || !var->is_empty());
//
//			// update current node
//			exec_tree_.AddToNodeStack(reploc);
//
//			MYLOG(2) << "Replaying path with forall thread node.";
//
//			return var;
//		}
//		safe_assert(exec_tree_.replay_path()->empty());
//	}

	//=======================================================

	if(group_.IsAllEnded()) {
		TRIGGER_BACKTRACK(THREADS_ALLENDED);
	}

	TransitionPredicatePtr _pred = pred;
	if(texpr != NULL) {
		_pred = (_pred && texpr);
	}

	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

	ForallThreadNode* select = NULL;
	node = exec_tree_.GetNextNodeInStack().parent();
	if(node != NULL) {
		select = ASINSTANCEOF(node, ForallThreadNode*);
		safe_assert(select != NULL && !select->covered());
//		if(select == NULL || select->covered()) {
//			safe_assert(select != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
		select->Init(scope, _pred);
	} else {
		select = new ForallThreadNode(static_info, scope, _pred);
	}

	safe_assert(select != NULL && !select->covered());

	// not covered yet

	// clear the var
	var = select->lvar();
	var->clear_thread();
	safe_assert(var != NULL && var->is_empty());

	select->OnSubmitted();

	// set atomic_ref to point to select
	exec_tree_.ReleaseRef(select);

	//=======================================================
	// wait for the consumption

	node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

#ifdef SAFE_ASSERT
	// check if correctly consumed
	ChildLoc last = exec_tree_.GetLastNodeInStack();
	safe_assert(select == ASINSTANCEOF(last.parent(), ForallThreadNode*));
	int child_index = last.child_index();
	safe_assert(child_index >= 0);
	safe_assert(var.get() == select->lvar().get());
	safe_assert(var->thread() == select->var(child_index)->thread());
#endif

	safe_assert(var != NULL || !(var->is_empty()));

	MYLOG(2) << "Added DSLForallThread: " << var->thread()->tid();

	exec_tree_.ReleaseRef(NULL);

	return var;
}

/********************************************************************************/

ThreadVarPtr Scenario::DSLExistsThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const char* message /*= NULL*/) {
	return DSLExistsThread(static_info, scope, PTRUE, ANY_THREAD, message);
}

ThreadVarPtr Scenario::DSLExistsThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred /*= TransitionPredicatePtr()*/, const char* message /*= NULL*/) {
	return DSLExistsThread(static_info, scope, pred, ANY_THREAD, message);
}

ThreadVarPtr Scenario::DSLExistsThread(StaticDSLInfo* static_info, ThreadVarPtrSet* scope, const TransitionPredicatePtr& pred /*= TransitionPredicatePtr()*/, const ThreadExprPtr& texpr /*= ThreadExprPtr()*/, const char* message /*= NULL*/) {
	safe_assert(static_info != NULL);
	if(message != NULL) static_info->set_message(message);

	MYLOG(2) << "Adding DSLExistsThread";

	//=======================================================

	ThreadVarPtr var;

//	if(Config::TrackAlternatePaths && is_replaying()) {
//		ChildLoc reploc = exec_tree_.replay_path()->pop();
//		safe_assert(reploc.parent() != NULL);
//		safe_assert(BETWEEN(0, reploc.child_index(), 1));
//		ExistsThreadNode* select = ASINSTANCEOF(reploc.parent(), ExistsThreadNode*);
//		safe_assert(select != NULL);
//
//		int child_index = reploc.child_index();
//		// if index is -1, then we should submit this select thread
//		if(child_index >= 0) {
//			// re-select the thread to update the associated thread variable
//			select->Update(pred);
//			var = select->var();
//			safe_assert(var != NULL || !var->is_empty());
//
//			// update current node
//			exec_tree_.AddToNodeStack(reploc);
//
//			MYLOG(2) << "Replaying path with forall thread node.";
//			return var;
//		}
//		safe_assert(exec_tree_.replay_path()->empty());
//	}

	//=======================================================

	if(group_.IsAllEnded()) {
		TRIGGER_BACKTRACK(THREADS_ALLENDED);
	}

	TransitionPredicatePtr _pred = pred;
	if(texpr != NULL) {
		_pred = (_pred && texpr);
	}

	ExecutionTree* node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

	ExistsThreadNode* select = NULL;
	node = exec_tree_.GetNextNodeInStack().parent();
	if(node != NULL) {
		select = ASINSTANCEOF(node, ExistsThreadNode*);
		safe_assert(select != NULL && !select->covered());
//		if(select == NULL || select->covered()) {
//			safe_assert(select != NULL || exec_tree_.IS_ENDNODE(node));
//			// release lock
//			exec_tree_.ReleaseRef(NULL);
//			// backtrack
//			TRIGGER_BACKTRACK(TREENODE_COVERED);
//		}
		select->Init(scope, _pred);
	} else {
		select = new ExistsThreadNode(static_info, scope, _pred);
	}

	safe_assert(select != NULL && !select->covered());

	// not covered yet

	// clear the var
	var = select->lvar();
	var->clear_thread();
	safe_assert(var != NULL && var->is_empty());

	select->OnSubmitted();

	// set atomic_ref to point to select
	exec_tree_.ReleaseRef(select);

	//=======================================================
	// wait for the consumption

	node = exec_tree_.AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(node == NULL);

#ifdef SAFE_ASSERT
	// check if correctly consumed
	ChildLoc last = exec_tree_.GetLastNodeInStack();
	safe_assert(select == ASINSTANCEOF(last.parent(), ExistsThreadNode*));
	safe_assert(last.child_index() == 0);
	safe_assert(var.get() == select->lvar().get());
#endif

	safe_assert(var != NULL || !(var->is_empty()));

	MYLOG(2) << "Added DSLExistsThread: " << var->thread()->tid();

	exec_tree_.ReleaseRef(NULL);


	return var;
}

/********************************************************************************/

} // end namespace
