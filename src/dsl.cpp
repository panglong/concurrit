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
	// sets root as the current parent, and adds it to the path
	current_path_.clear();
	AddToPath(&root_node_, 0);

	SetRef(NULL);
	safe_assert(GetRef() == NULL || REF_ENDTEST(GetRef()));
}

/*************************************************************************************/

// operations by clients

// run by test threads to get the next transition node
ExecutionTree* ExecutionTreeManager::AcquireRef(AcquireRefMode mode) {
	long nano_sec = 1L;
	while(true) {
		ExecutionTree* node = ExchangeRef(&lock_node_);
		if(REF_EMPTY(node)) {
			if(mode == EXIT_ON_EMPTY || mode == EXIT_ON_LOCK) {
				return NULL;
			} else {
				// release
				SetRef(NULL);
			}
		}
		else
		if(REF_LOCKED(node)) {
			// noop
		}
		else
		if(REF_ENDTEST(node)) {
			SetRef(node);
			return node; // indicates end of the test
		}
		else // full: non-null, non-lock, non-end
		if(mode == EXIT_ON_FULL) {
			// handle this
			safe_assert(node != NULL);
			return node;
		}
		else
		if(mode == EXIT_ON_LOCK) {
			return node;
		}
		// retry
		Thread::Yield(true);

		nano_sec <<= 1;
		if(nano_sec >= 999999999L) {
			// TODO(elmas): alert, backtrack!
			fprintf(stderr, "Failing on mode %d\n", mode);
			safe_assert(false);
		}
		if(nano_sec > (1L << 5)) {
			short_sleep(nano_sec, false);
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
		// release
		SetRef(node);
	}
}

/*************************************************************************************/

void ExecutionTreeManager::AddToPath(ExecutionTree* node, int child_index) {
	if(!current_path_.empty()) {
		ChildLoc parent = GetLastInPath();
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
	EndNode* end_node = NULL;
	// wait until the last node is consumed (or an end node is inserted)
	ExecutionTree* node = AcquireRef(EXIT_ON_EMPTY);
	if(REF_ENDTEST(node)) {
		// put back the node
		ReleaseRef(node);
		end_node = static_cast<EndNode*>(node);
	} else {
		// locked, create a new end node and set it
		end_node = new EndNode();
		// add to the path and set to ref
		ReleaseRef(end_node, 0);
	}
}

/*************************************************************************************/

void ExecutionTreeManager::EndWithException(Coroutine* current, std::exception* exception) {
	EndNode* end_node = NULL;
	// wait until we lock the atomic_ref, but the old node can be null or any other node
	ExecutionTree* node = AcquireRef(EXIT_ON_LOCK);
	if(REF_ENDTEST(node)) {
		// put back the node
		ReleaseRef(node);
		end_node = static_cast<EndNode*>(node);
	} else {
		// locked, create a new end node and set it
		end_node = new EndNode();
		// add to the path and set to ref
		ReleaseRef(end_node, 0);
	}
	safe_assert(end_node != NULL);
	// add my exception to the end node
	end_node->add_exception(exception, current, "EndWithException");
}


} // end namespace


