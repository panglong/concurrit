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

const char* CONCURRIT_HOME = NULL;

volatile bool IsInitialized = false;

void BeginCounit() {
	safe_assert(IsInitialized == false);

	printf("Initializing Concurrit\n");

	// init pth
//	int pth_init_result = pth_init();
//	safe_assert(pth_init_result == TRUE);

	// init work dir
	CONCURRIT_HOME = getenv("CONCURRIT_HOME");

	// init logging
	google::InitGoogleLogging("concurrit");

	Thread::init_tls_key();

	CoroutineGroup::init_main();

	do { // need a fence here
		IsInitialized = true;
	} while(false);
}

void EndCounit() {
	safe_assert(IsInitialized == true);

	printf("Finalizing Concurrit\n");

	CoroutineGroup::delete_main();

	Thread::delete_tls_key();

//	int pth_kill_result = pth_kill();
//	safe_assert(pth_kill_result == TRUE);

	do { // need a fence here
		IsInitialized = false;
	} while(false);
}

ConcurritInitializer::ConcurritInitializer() {
	BeginCounit();
}
ConcurritInitializer::~ConcurritInitializer() {
	EndCounit();
}

/********************************************************************************/

/*
 * Scenario
 */

UntilStarCondition TransferCriteria::until_star_;
UntilFirstCondition TransferCriteria::until_first_;
UntilEndCondition TransferCriteria::until_end_;

Scenario::Scenario(const char* name) {
	// sanity checks for initialization
	safe_assert(CoroutineGroup::main() != NULL);

	schedule_ = NULL;

	name_ = CHECK_NOTNULL(name);

	// schedule and group are initialized
	group_.set_scenario(this);
	explore_type_ = EXISTS;

	transfer_criteria_.Reset();

	// Scenario provides the default yield implementation
	yield_impl_ = static_cast<YieldImpl*>(this);

	dpor_enabled_ = true;

	statistics_ = boost::shared_ptr<Statistics>(new Statistics());

	test_status_ = TEST_BEGIN;
}

/********************************************************************************/

Scenario::~Scenario() {
	if(schedule_ != NULL) {
		delete schedule_;
	}
}

/********************************************************************************/

Scenario* Scenario::GetInstance() {
	// get current coroutine
	Coroutine* current = Coroutine::Current();
	safe_assert(current != NULL);

	CoroutineGroup* group = current->group();
	safe_assert(group != NULL);

	Scenario* scenario = group->scenario();
	safe_assert(scenario != NULL);

	return scenario;
}

/********************************************************************************/

void Scenario::LoadScheduleFromFile(const char* filename) {
	CHECK(filename != NULL) << "Filename is NULL!";
	if(schedule_ == NULL) {
		schedule_ = new Schedule();
	}
	VLOG(2) << SC_TITLE << "Loading schedule from file " << filename;
	schedule_->LoadFromFile(filename);
	VLOG(2) << SC_TITLE << "Loaded schedule from file " << filename;
}

/********************************************************************************/
Coroutine* Scenario::CreateThread(int id, ThreadEntryFunction function, void* arg, bool transfer_on_start /*= false*/) {
	char buff[MAX_THREAD_NAME_LENGTH];
	sprintf(buff, "THR-%d", id);
	return CreateThread(buff, function, arg, transfer_on_start);
}

// create a new thread or fetch the existing one, and start it.
Coroutine* Scenario::CreateThread(const char* name, ThreadEntryFunction function, void* arg, bool transfer_on_start /*= false*/) {
	std::string strname = std::string(name);
	Coroutine* co = group_.GetMember(strname);
	if(co == NULL) {
		// create a new thread
		VLOG(2) << SC_TITLE << "Creating new coroutine " << name;
		co = new Coroutine(name, function, arg);
		co->set_transfer_on_start(transfer_on_start);
		group_.AddMember(co);
	} else {
		VLOG(2) << SC_TITLE << "Re-creating and restarting coroutine " << name;
		if(co->status() <= PASSIVE) {
			printf("Cannot create new thread with the same name!");
			_Exit(UNRECOVERABLE_ERROR);
		}
		// update the function and arguments
		co->set_entry_function(function);
		co->set_entry_arg(arg);
		co->set_transfer_on_start(transfer_on_start);
	}
	// start it. in the usual case, waits until a transfer happens, or starts immediatelly depending ont he argument transfer_on_start
	co->Start();

	return co;
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

	Result* result = NULL;

	for(;true;) {

		/************************************************************************/
		for(;true;) {
			try {

				Start();

				VLOG(2) << SC_TITLE << "Starting path " << statistics_->counter("Num paths explored");

				std::exception* exc = RunOnce();

				// RunOnce should not throw an exception, so throw it here
				if(exc != NULL) {
					throw exc;
				}

				statistics_->counter("Num paths explored").increment();

				if(explore_type_ == EXISTS) {
					result = new ExistsResult(schedule_->Clone());
					break;
				} else {
					if(result == NULL) {
						result = new ForallResult();
					}

					if(ConcurritExecutionMode == PREEMPTIVE) {
						printf("Explored new path of length %d\n", exec_tree_.current_path()->size());
					}

					ASINSTANCEOF(result, ForallResult*)->AddSchedule(schedule_->Clone());
					VLOG(2) << "Found one path for forall search.";
					TRIGGER_WRAPPED_BACKTRACK("Forall");
				}

			} catch(std::exception* e) {
				CounitException* ce = ASINSTANCEOF(e, CounitException*);
				if(ce->is_backtrack()) {
					VLOG(2) << SC_TITLE << "Backtracking: " << e->what();

					// is backtrack is for terminating the search, exit the search loop
					if(INSTANCEOF(ce->cause(), TerminateSearchException*)) {
						goto LOOP_DONE; // break the outermost loop
					}

					if(Backtrack()) {
						continue;
					} else {
						VLOG(2) << SC_TITLE << "No feasible execution!!!";

						if(result == NULL) {
							result = new NoFeasibleExecutionResult(schedule_->Clone());
						} else {
							safe_assert(INSTANCEOF(result, ForallResult*));
						}
						goto LOOP_DONE; // break the outermost loop
					}

				} else if(ce->is_assertion_violation()){
					VLOG(2) << SC_TITLE << "Assertion is violated!";
					VLOG(2) << ce->what();

					safe_assert(INSTANCEOF(ce->cause(), AssertionViolationException*));
					result = new AssertionViolationResult(static_cast<AssertionViolationException*>(ce->cause()), schedule_->Clone());
				} else if(ce->is_internal()){
					VLOG(2) << SC_TITLE << "Internal exception thrown!";
					VLOG(2) << ce->what();

					safe_assert(INSTANCEOF(ce->cause(), InternalException*));
					throw ce->cause();
				} else {
					safe_assert(ce->cause() != NULL);
					VLOG(2) << SC_TITLE << "Exception caught: " << ce->cause()->what();

					safe_assert(INSTANCEOF(ce->cause(), std::exception*));
					result = new RuntimeExceptionResult(ce->cause(), schedule_->Clone());
				}
				break;
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

	Finish(result); // deletes schedule_

	safe_assert(result != NULL);
	return result;
}


/********************************************************************************/

std::exception* Scenario::RunOnce() throw() {

	RunSetUp();

	std::exception* exc = RunTestCase();

	RunUncontrolled();

	RunTearDown();

	test_status_ = TEST_ENDED;

	return exc;
}

/********************************************************************************/

void Scenario::RunUncontrolled() {
	// set uncontrolled run flag
	test_status_ = TEST_UNCONTROLLED;
	// set ended node to the execution tree
	exec_tree_.SetEnded();

	//---------------------------

	VLOG(2) << "Starting uncontrolled run";

	// start waiting all to end
	group_.WaitForAllEnd();

	VLOG(2) << "Ending uncontrolled run";
}

/********************************************************************************/

/**
 * Functions to run setup, teardown and testcase and get the exception.
 */
void Scenario::RunSetUp() throw() {
	test_status_ = TEST_SETUP;
	try {
		SetUp();
	} catch(...) { // } catch(std::exception* e) {
		fprintf(stderr, "Exceptions in SetUp are not allowed!!!\n");
		_Exit(UNRECOVERABLE_ERROR);
		// TRIGGER_WRAPPED_EXCEPTION("Setup", e);
	}
}

/********************************************************************************/

void Scenario::RunTearDown() throw() {
	test_status_ = TEST_TEARDOWN;
	try {
		TearDown();
	} catch(...) { // } catch(std::exception* e) {
		fprintf(stderr, "Exceptions in TearDown are not allowed!!!\n");
		_Exit(UNRECOVERABLE_ERROR);
		// TRIGGER_WRAPPED_EXCEPTION("Teardown", e);
	}
}

/********************************************************************************/

std::exception* Scenario::RunTestCase() throw() {
	test_status_ = TEST_CONTROLLED;
	try {
		try {
			TestCase();
		} catch(std::exception* e) {
			return WRAP_EXCEPTION("TestCase", e);
		}
	} catch(...) { // } catch(std::exception* e) {
		fprintf(stderr, "Exceptions other than std::exception in TestCase are not allowed!!!\n");
		_Exit(UNRECOVERABLE_ERROR);
		// TRIGGER_WRAPPED_EXCEPTION("TestCase", e);
	}
	return NULL;
}

/********************************************************************************/

void Scenario::ResolvePoint(SchedulePoint* point) {
		YieldPoint* ypoint = point->AsYield();
		if(!ypoint->IsResolved()) {
			std::string* strsource = reinterpret_cast<std::string*>(ypoint->source());
			safe_assert(strsource != NULL);
			Coroutine* source = group_.GetMember(*strsource);
			safe_assert(source != NULL);
			ypoint->set_source(source);
			ypoint->set_is_resolved(true);
			delete strsource;
		}
		if(point->IsTransfer()) {
			TransferPoint* transfer = point->AsTransfer();
			if(!transfer->IsResolved()) {
				std::string* strtarget = reinterpret_cast<std::string*>(transfer->target());
				safe_assert(strtarget != NULL);
				Coroutine* target = group_.GetMember(*strtarget);
				safe_assert(target != NULL);
				transfer->set_target(target);
				transfer->set_is_resolved(true);
				delete strtarget;
			}
		}
	}

/********************************************************************************/

bool Scenario::Backtrack() {
	if(ConcurritExecutionMode == COOPERATIVE) {
		return DoBacktrackCooperative();
	} else {
		return DoBacktrackPreemptive();
	}
}

/********************************************************************************/

bool Scenario::DoBacktrackCooperative() {
	VLOG(2) << "Start schedule: " << schedule_->ToString();

	// remove all points after current (inclusively).
	schedule_->RemoveCurrentAndBeyond();
	for(SchedulePoint* point = schedule_->RemoveLast(); point != NULL; point = schedule_->RemoveLast()) {

		if(point->IsChoice()) {
			// check if there is any more choice
			if(point->AsChoice()->ChooseNext()) {
				schedule_->AddLast(point);

				VLOG(2) << "End schedule: " << schedule_->ToString();
				return true;
			}
		}
		else if(point->IsTransfer()) {
			TransferPoint* transfer = point->AsTransfer();
			safe_assert(transfer->IsResolved());

			// if already backtrack point, decrease the count
			if(transfer->target() == NULL) {
				safe_assert(transfer->AsYield()->free_target());
				if(transfer->yield()->free_count()) {
					unsigned int count = transfer->yield()->count();
					if(--count > 0) {
						transfer->yield()->set_count(count);
						schedule_->AddLast(transfer);

						VLOG(2) << "End schedule: " << schedule_->ToString();
						return true;
					}
				}
			} else if(transfer->AsYield()->free_target()) {
				safe_assert(!transfer->enabled()->empty());
				// if it has still more choices, make it backtrack point
				if(transfer->has_more_targets(dpor_enabled_)) {
					// makes this a backtrack point (adds target to done and makes target NULL)
					transfer->make_backtrack_point();
					schedule_->AddLast(transfer);

					VLOG(2) << "End schedule: " << schedule_->ToString();
					return true;
				}
			}
		//************************
		// this is not a transfer point, but a plain yield point
		} else {
			safe_assert(point->AsYield()->count() > 0);

			Coroutine* target = NULL;
			if(!point->AsYield()->free_target()) {
				safe_assert(point->AsYield()->label() != MAIN_NAME && !point->AsYield()->source()->IsMain());
				target = group_.main();
			}

			// create a transferpoint
			TransferPoint* transfer = new TransferPoint(point->AsYield(), target);

			ResolvePoint(transfer);
			schedule_->AddLast(transfer);
			return true;
		}
		// delete the thrown-away yield point
		VLOG(2) << "Throwing away yield point: " << point->ToString();
		delete point;
	} // end loop
	return false; // no feasible execution
}

/********************************************************************************/

// although Start and Restart have the same code,
// we do not directly call Restart, since subclasses may override Start and Restart differently
void Scenario::Start() {
	safe_assert(test_status_ == TEST_BEGIN || test_status_ == TEST_ENDED);

	if(test_status_ == TEST_BEGIN) {
		// first start
		// reset statistics
		statistics_->Reset();
		statistics_->timer("Search time").start();
	} else {
		// restart
		exec_tree_.Restart();
	}

	test_status_ = TEST_BEGIN;

	if(schedule_ == NULL) {
		schedule_ = new Schedule();
	} else {
		schedule_->Restart();
	}

	// reset vc tracker
	vcTracker_.Restart();

	group_.Restart(); // restarts only already started coroutines

	transfer_criteria_.Reset();
}

/********************************************************************************/

void Scenario::Finish(Result* result) {
	safe_assert(test_status_ == TEST_ENDED);
	test_status_ = TEST_TERMINATED;

	if(schedule_ != NULL) {
		delete schedule_;
		schedule_ = NULL;
	}

	group_.Finish();

	// finish timer
	statistics_->timer("Search time").stop();

	// copy statistics to the result
	result->set_statistics(statistics_);

	std::cout << "********** Statistics **********" << std::endl;
	std::cout << statistics_->ToString() << std::endl;
}


/********************************************************************************/

TransferPoint* Scenario::OnYield(SchedulePoint* spoint, Coroutine* target) {

	VLOG(2) << SC_TITLE << "OnYield starting";

	YieldPoint* point = spoint->AsYield();
	Coroutine* source = point->source();
	std::string label = point->label();

	safe_assert(point->IsResolved());
	safe_assert(point == source->yield_point()->AsYield());
	safe_assert(source != NULL && source != target);

	CoroutineGroup group = group_;
	Coroutine* main = group.main();

	bool from_main = point->source() == main;

	bool is_ending = (label == ENDING_LABEL);

	TransferPoint* transfer = NULL;

	// check if we have a matching one
	if(schedule_->HasCurrent()) {
		SchedulePoint* current = schedule_->GetCurrent();
		transfer = current->AsTransfer();
		if(transfer == NULL) {
			// this is a choice point
			safe_assert(current->IsChoice());
			// sanity check:
			safe_assert(current->AsChoice()->source() == source);
			// skip this yield point, since we are expecting a choice point before
			goto DONE;
		}

		VLOG(2) << SC_TITLE << "Processing transfer from the log: " << transfer->ToString();

		ResolvePoint(transfer);
		safe_assert(transfer->IsResolved());

		// if source of transfer is not the current source of yield then there is a problem, so backtrack
		if(transfer->yield()->source() != point->source()) {
			printf("Execution diverged from the recorded schedule. Backtracking...");
			TRIGGER_BACKTRACK();
		}

		target = transfer->target();
		safe_assert(target != source);

		if(from_main) {
			VLOG(2) << SC_TITLE << "From main";
			safe_assert(target != main);
			safe_assert(point->label() == MAIN_LABEL);

			if(transfer->yield()->label() != point->label()) {
				transfer = NULL;
				goto DONE;
			}

			// if backtrack point, choose a new target
			if(target == NULL) {
				safe_assert(!transfer->done()->empty());
				// choose the next target
				target = GetNextEnabled(transfer);
				if(target == NULL) {
					// no enable thread found, so backtrack
					TRIGGER_BACKTRACK();
				}
			}
			safe_assert(target != NULL);
			transfer->set_target(target);
		} else {
			VLOG(2) << SC_TITLE << "From non-main";
			safe_assert(target != NULL);
			CHECK(target == main); // this holds for now but, may be relaxed later
			safe_assert(point->label() != MAIN_LABEL);

			if(transfer->yield()->label() != point->label()) {
				transfer = NULL;
				goto DONE;
			}

			if(!CheckUntil(transfer)) {
				TRIGGER_BACKTRACK();
			}
		}

		safe_assert(target != NULL);
		safe_assert(transfer != NULL);

		// check if the target is enabled
		StatusType status = target->status();
		if(status < ENABLED && BLOCKED <= status) {
			TRIGGER_BACKTRACK();
		}

		VLOG(2) << SC_TITLE << "Consuming transfer.";
		if(!transfer->ConsumeOnce()) {
			// do not yield yet if not consumed all
			transfer = NULL;
			goto DONE;
		}

		// here we delete and reset the yield point of transfer
		// this helps to get a yield point with non-null loc and access
		delete transfer->yield();
		transfer->set_yield(point);

	//********************************************

	} else {
		// no more points in the schedule, so create the next new one

		VLOG(2) << SC_TITLE << "Generating a new transfer";

		if(from_main) {
			if(target == NULL) {
				target = GetNextEnabled();
				if(target == NULL) {
					// no enabled thread found, so backtrack
					TRIGGER_BACKTRACK();
				}
			}
			safe_assert(target != main);
		} else {
			safe_assert(target == main);

			if(!CheckUntil(point)) {
				// else, choose not to continue, so do not yield
				transfer = NULL;
				goto DONE;
			}
		}
		if(point->IsTransfer()) {
			transfer = point->AsTransfer();
		} else {
			transfer = new TransferPoint(point, target);
			// consume all (because this transfer is made from a yield point which is already consumed)
			transfer->ConsumeAll();
			// resolve it
			ResolvePoint(transfer);
		}
		safe_assert(transfer->IsResolved());
		schedule_->AddCurrent(transfer, false); // do not move current yet

	}
	//********************************************

DONE:
	// after this point, transfer does not change from non-null to null

	VLOG(2) << "DONE. Scenario::OnYield chose " << ((transfer != NULL) ? transfer->ToString() : "NULL");
	if(transfer == NULL) { // no transfer, this may be because the current point is a choice point
		// if yield of source is a transfer point, then we should not be here
		safe_assert(!source->yield_point()->IsTransfer());

		// if the ending yield and transfer is still NULL, then there is a problem, so we should backtrack
		if(is_ending) {
			TRIGGER_BACKTRACK();
		}

		// update schedule to add this yield point
		if(schedule_->GetCurrent() != point) {
			VLOG(2) << SC_TITLE << "Adding yield point " << point->ToString() << " to schedule";
			schedule_->AddCurrent(point, true);
		}

	} else { // will take the transfer
		// transfer is to be taken, so consume the current one
		safe_assert(schedule_->GetCurrent() == transfer);
		schedule_->ConsumeCurrent(); // shift the current pointer
		source->set_yield_point(transfer); // will override point

		safe_assert(transfer->target() != NULL);

		if(!from_main) {
			// clear the until condition and except set if we are transferring
			transfer_criteria_.Reset();
		}

		// target should have already been set.
		target = transfer->target();
		safe_assert(target != NULL);

		// set next and previous pointers here
		SchedulePoint* target_point = target->yield_point();
		if(target_point != NULL) {
			YieldPoint* target_yield = target_point->AsYield();
			safe_assert(target_point->IsTransfer());
			transfer->set_next(target_yield);
			target_yield->set_prev(transfer);
		}

		//-----------------------------------------
		// updating transfer for DPOR search
		// we do this only when transfer is the last one in the schedule, means that we have just chosen a new target
		if(schedule_->GetLast() == transfer) {
			// TODO(elmas): optimize: do not set all the fields for all kinds of transfers

			// update enabled coroutines
			CoroutinePtrSet enabled = group_.GetEnabledSet();
			transfer->set_enabled(enabled);

			// if this is the first time of transfer, add target to backtrack
			if(transfer->is_first_transition()) {
				safe_assert(transfer->done()->empty());
				safe_assert(transfer->backtrack()->empty());
				transfer->backtrack()->insert(target);
			}

			// update done set (do this at the end)
			transfer->done()->insert(target);
		}
		// sanity checks
		safe_assert(!transfer->is_first_transition());
		// make sure that target is already in done
		safe_assert(transfer->done()->find(target) != transfer->done()->end());
	}

	// ensure that if the yield label is "ending", we are transferring (transfer must not be NULL)
	safe_assert(point->label() != ENDING_LABEL || transfer != NULL);

	return transfer;
}

/********************************************************************************/

// Note: this should not update transfer, as we may not have decided on choosing transfer yet
Coroutine* Scenario::GetNextEnabled(TransferPoint* transfer /*= NULL*/) {
	CoroutinePtrSet except_targets;
	CoroutinePtrSet* backtrack = NULL;
	if(transfer != NULL) {
		// if first transition, then we choose a random next target
		// otherwise, we choose a target from backtrack
		if(!transfer->is_first_transition()) {
			CoroutinePtrSet* done = transfer->done();
			safe_assert(!done->empty());
			except_targets.insert(done->begin(), done->end());

			if(dpor_enabled_) {
				// if not the first transition, we use backtrack set,
				// otherwise we chose a random next target
				backtrack = transfer->backtrack();
			}
		}
	}
	if(!transfer_criteria_.except()->empty()) {
		except_targets.insert(transfer_criteria_.except()->begin(), transfer_criteria_.except()->end());
	}
	return group_.GetNextEnabled((except_targets.empty() ? NULL : &except_targets), backtrack);
}

/********************************************************************************/

SchedulePoint* Scenario::TransferStar(SourceLocation* loc) {
	return this->Transfer(NULL, loc);
}

/********************************************************************************/

// when this begins, we have a particular target to transfer
// all nondeterminism about transfer is resolved in transfer star
SchedulePoint* Scenario::Transfer(Coroutine* target, SourceLocation* loc) {
	VLOG(2) << SC_TITLE << "Transfer request to " << ((target != NULL) ? target->name() : "star");

	CoroutineGroup group = group_;

	// get current coroutine
	Coroutine* current = Coroutine::Current();
	// sanity checks
	safe_assert(current != NULL);

	if(target != NULL) {
		safe_assert(group.GetMember(target->name()) == target);

		// backtrack if target is not enabled
		if(target->status() != ENABLED) {
			TRIGGER_BACKTRACK();
		}
	}

	// this coroutine is main, get the main coroutine from our group
	Coroutine* main = group.main();

	// check if this is not the main thread (main is not allowed to yield)
	CHECK(current == main) << "Transfer must be called from the main thread!";
	CHECK(target != main) << "Transfer must not be targeted to main!";

	std::string main_label(MAIN_LABEL);
	return do_yield(&group_, main, target, main_label, loc, NULL);
}

/********************************************************************************/

bool Scenario::AllEnded() {
	return group_.IsAllEnded();
}

/********************************************************************************/

Scenario* Scenario::UntilEnd() {
	transfer_criteria_.UntilEnd();
	return this;
}

/********************************************************************************/

Scenario* Scenario::UntilFirst() {
	transfer_criteria_.UntilFirst();
	return this;
}

/********************************************************************************/

Scenario* Scenario::UntilStar() {
	transfer_criteria_.UntilStar();
	return this;
}

/********************************************************************************/

Scenario* Scenario::Until(const char* label) {
	transfer_criteria_.Until(label);
	return this;
}

/********************************************************************************/

Scenario* Scenario::Until(UntilCondition* cond) {
	transfer_criteria_.Until(cond);
	return this;
}

/********************************************************************************/

Scenario* Scenario::Until(const std::string& label) {
	transfer_criteria_.Until(label);
	return this;
}

/********************************************************************************/

bool Scenario::CheckUntil(SchedulePoint* point) {
	safe_assert(transfer_criteria_.until() != NULL);
	return transfer_criteria_.until()->Check(point);
}

/********************************************************************************/

Scenario* Scenario::Except(Coroutine* t) {

	transfer_criteria_.Except(t);

	return this;
}

/********************************************************************************/

Scenario* Scenario::Except(const char* name) {
	Coroutine* t = NULL;

	if(name != NULL) {
		t = group_.GetMember(name);
		safe_assert(t != NULL);
	}

	return Except(t);
}

/********************************************************************************/

void Scenario::OnAccess(Coroutine* current, SharedAccess* access) {
	safe_assert(access != NULL);
	safe_assert(access == current->yield_point()->AsYield()->access()); // access must be the last access of current

	VLOG(2) << SC_TITLE << "Access by " << current->name() << ": " << access->ToString();

	// notify vc tracker
	vcTracker_.OnAccess(current->yield_point(), schedule_->coverage());
}

/********************************************************************************/

void Scenario::UpdateBacktrackSets() {
	// iterate over all coroutines in the group
	for(MembersMap::iterator itr = group_.members()->begin(); itr != group_.members()->end(); ++itr) {
		Coroutine* co = itr->second;
		if(co->status() < ENDED) {
			// handle its next access
			SchedulePoint* point = co->yield_point();
			if(point != NULL) {
				vcTracker_.UpdateBacktrackSets(point);
			}
		}
	}
}

/********************************************************************************/

SchedulePoint* yield(const char* label, SourceLocation* loc /*=NULL*/, SharedAccess* access /*=NULL*/, bool force /*=false*/) {

	// give warning if access is null
	if(access == NULL && strcmp(CHECK_NOTNULL(label), ENDING_LABEL) != 0) {
		VLOG(2) << "YIELD without a shared variable access.";
		if(loc != NULL) {
			VLOG(2) << "Related location: " << loc->ToString();
		}
	}

	// get current coroutine
	Coroutine* current = Coroutine::Current();
	safe_assert(current != NULL);

	CoroutineGroup* group = current->group();
	safe_assert(group != NULL);

	Coroutine* main = group->main();
	safe_assert(main != current);

	std::string strlabel = std::string(label);

	Scenario* scenario = group->scenario();
	safe_assert(scenario != NULL);

	// if forced yield, put an until condition for this
	if(force) {
		// not the ending label
		safe_assert(strcmp(label, ENDING_LABEL) != 0);
		// TODO(elmas): check if the current until condition is consistent with forced yield
		safe_assert(!current->IsMain());
		scenario->UntilFirst(); // choose the first yield
	}

	return scenario->do_yield(group, current, main, strlabel, loc, access);

}

/********************************************************************************/

void Scenario::ExhaustiveSearch() {
//	safe_assert(group_.current() == group_.main());

	TEST_FORALL();

	UNTIL_ALL_END {
		UNTIL_STAR()->TRANSFER_STAR();
	}
}

void Scenario::ContextBoundedExhaustiveSearch(int C) {
//	safe_assert(group_.current() == group_.main());
	CHECK(C > 0) << "Context bound must be > 0";

	TEST_FORALL();

	Coroutine* prev = NULL;
	UNTIL_ALL_END {
		SchedulePoint* point = UNTIL_STAR()->EXCEPT(prev)->TRANSFER_STAR();

		safe_assert(point->IsTransfer());
		safe_assert(point->AsTransfer()->target() == group_.main());

		prev = point->AsYield()->source();
		if(C > 0 && --C == 0) break;
	}

	FINISH_ALL();
}

void Scenario::NDSeqSearch() {
//	safe_assert(group_.current() == group_.main());

	TEST_FORALL();

	UNTIL_ALL_END {
		FINISH_STAR();
	}
}

/********************************************************************************/

void Scenario::RunSavedSchedule(const char* filename) {
	Schedule schedule(filename, true); // load it
	schedule_->extend_points(&schedule);
}

/********************************************************************************/

void BeginStrand(const char* name) {
	safe_assert(name != NULL);
	// printf("Resuming thread %s...\n", name);
}

/********************************************************************************/

SchedulePoint* Scenario::Yield(Scenario* scenario, CoroutineGroup* group, Coroutine* current, Coroutine* target, std::string& label, SourceLocation* loc, SharedAccess* access) {
	VLOG(2) << "Scenario::Yield (default implementation)";

	CHECK(scenario == this);

	// sanity checks
	safe_assert(current != NULL);
	safe_assert(group != NULL);
	safe_assert(scenario == group->scenario());
	safe_assert(group == scenario->group());

	VLOG(2) << "Yield request from " << current->name();

	// update backtrack set before taking the transition
	this->UpdateBacktrackSets();

	// generate or update the current yield point of current
	SchedulePoint* point = current->OnYield(target, label, loc, access);

	// make the next decision
	TransferPoint* transfer = this->OnYield(point, target);

	SchedulePoint* target_point = NULL;

	// point != NULL means go to the target
	// point == NULL means do not yield
	if(transfer != NULL) {
		safe_assert(transfer->IsResolved());
		safe_assert(transfer->AsYield()->source() == current);
		safe_assert(transfer->AsYield()->label() == label);
		safe_assert(transfer->rem_count() == 0);

		Coroutine* target = transfer->target();
		safe_assert(transfer->target() != NULL);

		// do the transfer
		current->Transfer(target, MSG_TRANSFER);

		// return yield point of the target
		target_point = target->yield_point();
	}

	//----------------------------------------
	// after transfer returns back to current

	return target_point; // means did not transfer to another thread
}

/********************************************************************************/

// program state should be updated before this point
void Scenario::BeforeControlledTransition(Coroutine* current) {
	if(test_status_ == TEST_UNCONTROLLED) return;

	printf("Before controlled transition by %s\n", current->name().c_str());

	// make thread blocked
	current->set_status(BLOCKED);


	//...

}

void Scenario::AfterControlledTransition(Coroutine* current) {
	if(test_status_ == TEST_UNCONTROLLED) return;

	printf("After controlled transition by %s\n", current->name().c_str());

	//...


	// remove all transition info records
	current->trinfolist()->clear();

	// make thread enabled
	current->set_status(ENABLED);
}

/********************************************************************************/

bool Scenario::DoBacktrackPreemptive() {
	ExecutionTree* root = exec_tree_.root_node();
	std::vector<ChildLoc>* path = exec_tree_.current_path();
	safe_assert(path->size() >= 1 && (*path)[0].parent() == root);
	// if path only contains root, then we are done
	if(path->back().parent() == root){
		safe_assert(path->size() == 1);
		root->set_covered(true); // mark the root covered
		return false;
	}

	// else, compute covered flags, if root is covered, then we are done

	// go back in the path computing coverage locally
	// check the unsatisfied node
	ExecutionTree* current_node = exec_tree_.GetRef();
	safe_assert(current_node == NULL || exec_tree_.REF_ENDTEST(current_node) || current_node == path->back());

	// set the child of the last node in the path covered
	ExecutionTree* child = path->back();
	if(child != NULL) {
		child->set_covered(true);
	} else {
		path->back().set(exec_tree_.covered_node());
	}

	// propagate coverage back in the path
	for(int i = path->size()-1; i >= 0; --i) {
		ExecutionTree* node = (*path)[i].parent();
		node->ComputeCoverage(true); //  do not recurse, only use immediate children
	}
	// check if the root is covered
	return !root->covered();
}

/********************************************************************************/

bool Scenario::DSLChoice() {
	VLOG(2) << "Adding DSLChoice";
	printf("Adding DSLChoice\n");

	bool ret = false;
	ExecutionTree* node = exec_tree_.GetNextTransition();

	ChoiceNode* choice = NULL;
	if(node != NULL) {
		safe_assert(exec_tree_.GetLastInPath().check(node));
		safe_assert(INSTANCEOF(node, ChoiceNode*));
		choice = ASINSTANCEOF(node, ChoiceNode*);
		if(choice->covered()) {
			// backtrack
			TRIGGER_BACKTRACK();
		}
	} else {
		ChildLoc parent = exec_tree_.GetLastInPath();
		choice = new ChoiceNode();
		parent.set(choice);
	}

	// not covered yet
	// check children
	ExecutionTree* child = choice->child(0);
	if(child == NULL || !child->covered()) {
		ret = true;
	} else {
		child = choice->child(1);
		safe_assert(child == NULL || !child->covered());
		ret = false;
	}

	exec_tree_.ConsumeTransition(choice, (ret ? 0 : 1));

	printf("DSLChoice returns %s\n", (ret ? "true" : "false"));

	return ret;
}

/********************************************************************************/

void Scenario::DSLTransition(TransitionPredicate* pred) {
	VLOG(2) << "Adding DSLTransition";

	// check if all threads terminated
	if(group_.IsAllEnded()) {
		TRIGGER_BACKTRACK();
	}

	ExecutionTree* node = exec_tree_.GetNextTransition();

	TransitionNode* trans = NULL;
	if(node != NULL) {
		safe_assert(INSTANCEOF(node, TransitionNode*));
		trans = ASINSTANCEOF(node, TransitionNode*);
		if(trans->covered()) {
			// backtrack
			TRIGGER_BACKTRACK();
		}
	} else {
		trans = new TransitionNode(pred, exec_tree_.GetLastInPath());
	}

	// new transition, next transition is child
//	exec_tree_.SetNextTransition(trans, trans->child());
}

/********************************************************************************/


} // end namespace
