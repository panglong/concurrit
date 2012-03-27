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

// extern'ed variables
TrueTransitionPredicate __true_transition_predicate__;
FalseTransitionPredicate __false_transition_predicate__;

TransitionPredicate* TransitionPredicate::True() { return &__true_transition_predicate__; }
TransitionPredicate* TransitionPredicate::False() { return &__false_transition_predicate__; }

/*************************************************************************************/

TPVALUE TPNOT(TPVALUE v) {
	static TPVALUE __not_table__ [3] = {
			TPTRUE, TPFALSE, TPUNKNOWN
	};
	safe_assert(v != TPINVALID);
	return __not_table__[v];
}

TPVALUE TPAND(TPVALUE v1, TPVALUE v2) {
	static TPVALUE __and_table__ [3][3] = {
	//		F			T			U
	/*F*/	{TPFALSE,   TPFALSE, 	TPFALSE},
	/*T*/	{TPFALSE,   TPTRUE, 	TPUNKNOWN},
	/*U*/	{TPFALSE, 	TPUNKNOWN, 	TPUNKNOWN}
	};
	safe_assert(v1 != TPINVALID && v2 != TPINVALID);
	return __and_table__[v1][v2];
}

TPVALUE TPOR(TPVALUE v1, TPVALUE v2) {
	static TPVALUE __or_table__ [3][3] = {
	//		F			T			U
	/*F*/	{TPFALSE,   TPTRUE,		TPUNKNOWN},
	/*T*/	{TPTRUE,   	TPTRUE, 	TPTRUE},
	/*U*/	{TPUNKNOWN,	TPTRUE,		TPUNKNOWN}
	};
	safe_assert(v1 != TPINVALID && v2 != TPINVALID);
	return __or_table__[v1][v2];
}

/*************************************************************************************/

ExecutionTree::~ExecutionTree(){
	VLOG(2) << "Deleting execution tree!";
	for_each_child(child) {
		if(child != NULL) {
			delete child;
		}
	}
}

/*************************************************************************************/

bool ExecutionTree::ContainsChild(ExecutionTree* node) {
	for_each_child(child) {
		if(child != NULL) {
			if(child == node) {
				return true;
			}
		}
	}
	return false;
}

/*************************************************************************************/

void ExecutionTree::InitChildren(int n) {
	children_.clear();
	for(int i = 0; i < n; ++i) {
		children_.push_back(NULL);
	}
}

/*************************************************************************************/

ExecutionTree* ExecutionTree::child(int i /*= 0*/) {
	safe_assert(0 <= i);
	if(i >= children_.size()) {
		return NULL;
	}
	return children_[i];
}

/*************************************************************************************/

void ExecutionTree::add_child(ExecutionTree* node) {
	if(!ContainsChild(node)) {
		children_.push_back(node);
	}
}

/*************************************************************************************/

void ExecutionTree::set_child(ExecutionTree* node, int i) {
	safe_assert(BETWEEN(0, i, children_.size()-1));
	children_[i] = node;
}

/*************************************************************************************/

ExecutionTree* ExecutionTree::get_child(int i) {
	safe_assert(BETWEEN(0, i, children_.size()-1));
	return children_[i];
}

/*************************************************************************************/

void ExecutionTree::ComputeCoverage(Scenario* scenario, bool recurse) {
	if(!covered_) {
		bool cov = true;
		for_each_child(child) {
			if(child != NULL) {
				if(!child->covered_) {
					if(recurse) {
						child->ComputeCoverage(scenario, recurse);
					}
					cov = cov && child->covered_;
				}
			} else {
				cov = false; // NULL child means undiscovered-yet branch
			}
		}
		covered_ = cov;
	}
}

/*************************************************************************************/

ExecutionTreeManager::ExecutionTreeManager() {
	root_node_.InitChildren(1);
	lock_node_.InitChildren(0);

	Restart();
}

/*************************************************************************************/

void ExecutionTreeManager::Restart() {
	// sets root as the current parent, and adds it to the path
	current_path_.clear();
	AddToPath(&root_node_, 0);

	SetRef(NULL);
	safe_assert(GetRef() == NULL || REF_ENDTEST(GetRef()));
}

/*************************************************************************************/

// operations by clients

// only main can run this!!!
// this always sets the timeout
ExecutionTree* ExecutionTreeManager::AcquireRefEx(AcquireRefMode mode, long timeout_usec /*= -1*/) {
	safe_assert(Coroutine::Current()->IsMain());
	if(timeout_usec < 0) timeout_usec = 999999L;
	ExecutionTree* node = AcquireRef(mode, timeout_usec);
	if(REF_ENDTEST(node)) {
		// main has not ended the execution, so this must be due to an exception by another thread
		safe_assert(static_cast<EndNode*>(node)->exception() != NULL);
		TRIGGER_BACKTRACK(EXCEPTION);
	}
	return node;
}

// run by test threads to get the next transition node
// only main can set timeout
ExecutionTree* ExecutionTreeManager::AcquireRef(AcquireRefMode mode, long timeout_usec /*= -1*/) {
	safe_assert(timeout_usec <= 0 || Coroutine::Current()->IsMain());
	Timer timer;
	if(timeout_usec > 0) {
		timer.start();
	}

	while(true) {
		ExecutionTree* node = ExchangeRef(&lock_node_);
		if(REF_LOCKED(node)) {
			// noop
			Thread::Yield(true);
		}
		else
		if(REF_ENDTEST(node)) {
			SetRef(node);
			return node; // indicates end of the test
		}
		else
		if(mode == EXIT_ON_LOCK) {
			return node; //  can be null, but cannot be lock or end node
		}
		else {
			if(REF_EMPTY(node)) {
				if(mode == EXIT_ON_EMPTY) {
					// will process this
					return NULL;
				} else {
					SetRef(NULL);
				}
			}
			else // FULL
			if(mode == EXIT_ON_FULL) {
				// will process this
				safe_assert(node != NULL);
				return node;
			} else {
				// release
				SetRef(node);
			}

			//=========================================
			// yield and wait
			if(timeout_usec > 0) {
				VLOG(2) << "Timed wait in AcquireRef";
				int result = sem_ref_.WaitTimed(timeout_usec);
				if(result == ETIMEDOUT) {
					// fire timeout (backtrack)
					TRIGGER_BACKTRACK(TIMEOUT);
				}
				safe_assert(result == PTH_SUCCESS);
			} else {
				VLOG(2) << "Untimed wait in AcquireRef";
				sem_ref_.Wait();
			}
			sem_ref_.Signal();
		}

		//=========================================
		// retry
		// check timer
		if(timeout_usec > 0) {
			if(timer.getElapsedTimeInMicroSec() > timeout_usec) {
				// fire timeout (backtrack)
				TRIGGER_BACKTRACK(TIMEOUT);
			}
		}
	}
	// unreachable
	safe_assert(false);
	return NULL;
}

/*************************************************************************************/

// TODO(elmas): no need for GetRef, just SetRef is enough if the assertion is valid
void ExecutionTreeManager::ReleaseRef(ExecutionTree* node /*= NULL*/, int child_index /*= -1*/) {
	ExecutionTree* current_node = GetRef();
	safe_assert(REF_LOCKED(current_node) || REF_ENDTEST(current_node));

	if(child_index >= 0) {
		safe_assert(node != NULL);

		// put node to the path
		AddToPath(node, child_index);

		// if released node is an end node, we do not nullify atomic_ref
		SetRef(REF_ENDTEST(node) ? node : NULL);
	} else {
		safe_assert(REF_LOCKED(current_node) && !REF_ENDTEST(current_node));
		// release
		SetRef(node);
	}
	sem_ref_.Signal();
//	Thread::Yield(true);
}

/*************************************************************************************/

void ExecutionTreeManager::AddToPath(ExecutionTree* node, int child_index) {
	if(!current_path_.empty()) {
		ChildLoc parent = GetLastInPath();
		if(REF_ENDTEST(node) && parent.get() != NULL) {
			// set old_root of end node
			static_cast<EndNode*>(node)->set_old_root(parent.get());
		} else {
			// if node is not end node, the we are either overwriting NULL or the same value
			safe_assert(parent.check(NULL) || parent.check(node));
		}
		parent.set(node);
	}
		// put node to the path
	current_path_.push_back({node, child_index});
}

/*************************************************************************************/

ExecutionTree* ExecutionTreeManager::GetRef() {
	return static_cast<ExecutionTree*>(atomic_ref_.load());
}

ExecutionTree* ExecutionTreeManager::ExchangeRef(ExecutionTree* node) {
	return static_cast<ExecutionTree*>(atomic_ref_.exchange(node));
}

// normally end node cannot be overwritten, unless overwrite_end flag is set
void ExecutionTreeManager::SetRef(ExecutionTree* node) {
	atomic_ref_.store(node);
}

/*************************************************************************************/

ChildLoc ExecutionTreeManager::GetLastInPath() {
	safe_assert(!current_path_.empty());
	return current_path_.back();
}

/*************************************************************************************/

bool ExecutionTreeManager::CheckEndOfPath(std::vector<ChildLoc>* path /*= NULL*/) {
	if(path == NULL) {
		path = &current_path_;
	}

	safe_assert(path->size() >= 2);
	safe_assert((*path)[0].parent() == &root_node_);
	safe_assert(REF_ENDTEST(path->back()));
	safe_assert(GetRef() != NULL);
	safe_assert(path->back() == GetRef());

	return true;
}

/*************************************************************************************/

void ExecutionTreeManager::EndWithSuccess() {
	// wait until the last node is consumed (or an end node is inserted)
	// we use AcquireRefEx to use a timeout to check if the last-inserted transition was consumed on time
	// in addition, if there is already an end node, AcquireRefEx throws a backtrack
	ExecutionTree* node = AcquireRefEx(EXIT_ON_EMPTY);
	safe_assert(!REF_ENDTEST(node));
	// locked, create a new end node and set it
	// also adds to the path and set to ref
	ReleaseRef(new EndNode(), 0);
}

/*************************************************************************************/

// (do not use AcquireRefEx here)
void ExecutionTreeManager::EndWithException(Coroutine* current, std::exception* exception, const std::string& where /*= "<unknown>"*/) {
	EndNode* end_node = NULL;
	// wait until we lock the atomic_ref, but the old node can be null or any other node
	ExecutionTree* node = AcquireRef(EXIT_ON_LOCK);
	if(REF_ENDTEST(node)) {
		end_node = static_cast<EndNode*>(node);
	} else {
		// locked, create a new end node and set it
		end_node = new EndNode();
		// add to the path and set to ref
		ReleaseRef(end_node, 0);
	}
	safe_assert(end_node != NULL);
	// add my exception to the end node
	end_node->add_exception(exception, current, where);
}

/*************************************************************************************/


void ExecutionTreeManager::EndWithBacktrack(Coroutine* current, const std::string& where) {
	EndWithException(current, GetBacktrackException(), where);
}


/*************************************************************************************/

TransitionConstraint::TransitionConstraint(Scenario* scenario)
: scenario_(scenario) {
	scenario_->trans_constraints()->push_back(this);
}

TransitionConstraint::~TransitionConstraint(){
	safe_assert(scenario_->trans_constraints()->back() == this);
	scenario_->trans_constraints()->pop_back();
}

/*************************************************************************************/

void SelectThreadNode::ComputeCoverage(Scenario* scenario, bool recurse) {
	safe_assert(scenario != NULL);
	// this check is important, because we should not compute coverage at all if already covered
	// since the computation below may turn already covered not covered
	if(!covered_) {
		ExecutionTree::ComputeCoverage(scenario, recurse);
		covered_ = covered_ && (children_.size() == scenario->group()->GetNumMembers());
	}
}

/*************************************************************************************/

ThreadVar::~ThreadVar() {
	// nullify the variable pointer of the select node
	SelectThreadNode* select = ASINSTANCEOF(select_node_.parent(), SelectThreadNode*);
	// selectthread node may be null, if not satisfied at all
	if(select != NULL) {
		select->set_var(NULL);
	}
}

// returns the coroutine selected, if any
Coroutine* ThreadVar::thread() {
	// do not call before thread is selected
	safe_assert(!select_node_.empty());

	SelectThreadNode* select = ASINSTANCEOF(select_node_.parent(), SelectThreadNode*);
	safe_assert(select != NULL);
	safe_assert(select->var() == this);
	return select->get_selected_thread();
}

} // end namespace


