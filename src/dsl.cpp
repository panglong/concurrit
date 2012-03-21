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

void ExecutionTree::ComputeCoverage(bool recurse) {
	if(!covered_) {
		bool cov = true;
		for_each_child(child) {
			if(child != NULL) {
				if(!child->covered_) {
					if(recurse) {
						child->ComputeCoverage(recurse);
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
	SetRef(NULL);
	current_node_ = NULL;
	current_path_.clear();
	current_path_.push_back(&root_node_);
}

/*************************************************************************************/

// operations by script
ExecutionTree* ExecutionTreeManager::GetNextTransition() {
	// wait until atomic_ref is not empty
	while(true) {
		ExecutionTree* node = GetRef();
		if(REF_EMPTY(node)) {
			// we obtained the lock!
			return current_node_;
		}
		safe_assert(REF_LOCKED(node));
		Thread::Yield(true);
	}
	// unreachable
	safe_assert(false);
	return NULL;
}

/*************************************************************************************/

void ExecutionTreeManager::SetNextTransition(ExecutionTree* node, ExecutionTree* next_node) {
	// we have already obtained the empty-lock!
	safe_assert(current_node_ == NULL || current_node_ == node);
	safe_assert(REF_EMPTY(GetRef()));

	// update current node pointer
	safe_assert(next_node == NULL || (node != NULL && next_node->parent() == node));
	safe_assert(next_node == NULL || (node != NULL && node->ContainsChild(next_node)));
	current_node_ = next_node;

	// release
	SetRef(node);
}

/*************************************************************************************/

// operations by clients

// run by test threads to get the next transition node
ExecutionTree* ExecutionTreeManager::AcquireNextTransition() {
	while(true) {
		ExecutionTree* node = ExchangeRef();
		if(REF_EMPTY(node)) {
			// release
			SetRef(NULL);
		}
		else
		if(REF_LOCKED(node)) {
			// noop
		}
		else {
			// handle this
			safe_assert(node != NULL);
			return node;
		}
		// retry
		Thread::Yield(true);
	}
	// unreachable
	safe_assert(false);
	return NULL;
}

/*************************************************************************************/

void ExecutionTreeManager::ReleaseNextTransition(ExecutionTree* node, bool consumed) {
	safe_assert(node != NULL);
	safe_assert(REF_LOCKED(GetRef()));

	if(consumed) {
		ConsumeTransition(node);
	}

	// release
	SetRef(consumed ? NULL : node);
}

/*************************************************************************************/

void ExecutionTreeManager::ConsumeTransition(ExecutionTree* node) {
	// put node to the path
	current_path_.push_back(node);
}

ExecutionTree* ExecutionTreeManager::GetRef() {
	return static_cast<ExecutionTree*>(atomic_ref_.load());
}

ExecutionTree* ExecutionTreeManager::ExchangeRef() {
	return static_cast<ExecutionTree*>(atomic_ref_.exchange(&lock_node_));
}

void ExecutionTreeManager::SetRef(ExecutionTree* node) {
	atomic_ref_.store(node);
}

ExecutionTree* ExecutionTreeManager::GetLastInPath() {
	safe_assert(!current_path_.empty());
	return current_path_.back();
}



} // end namespace


