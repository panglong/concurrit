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

int ExecutionTree::num_nodes_ = 0;

/*************************************************************************************/

ExecutionTree::~ExecutionTree(){
	VLOG(2) << "Deleting execution tree!";
	for_each_child(child) {
		if(child != NULL && !INSTANCEOF(child, EndNode*)) {
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

int ExecutionTree::index_of(ExecutionTree* node) {
	for(int i = 0, e = children_.size(); i < e; ++i) {
		if(children_[i] == node) {
			return i;
		}
	}
	return -1;
}

/*************************************************************************************/

bool ExecutionTree::child_covered(int i /*= 0*/) {
	safe_assert(BETWEEN(0, i, children_.size()-1));
	ExecutionTree* c = child(i);
	return c != NULL && c->covered();
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

bool ExecutionTree::ComputeCoverage(bool recurse, bool call_parent /*= false*/) {
	if(!covered_) {
		bool cov = true;
		for_each_child(child) {
			if(child != NULL) {
				if(!child->covered_) {
					if(recurse) {
						child->ComputeCoverage(recurse);
					}
					cov = cov && child->covered_;
					if(!cov) break;
				}
			} else {
				cov = false; // NULL child means undiscovered-yet branch
				break;
			}
		}
		covered_ = cov;
	}
	if(covered_ && call_parent && parent_ != NULL) {
		parent_->ComputeCoverage(recurse, true);
	}
	return covered_;
}

/*************************************************************************************/

void ExecutionTree::PopulateLocations(ChildLoc& loc, std::vector<ChildLoc>* current_nodes) {
	if(ExecutionTreeManager::IS_SELECTNODE(this) && !this->covered_) {
		int sz = children_.size();
		int child_index = loc.child_index();
		safe_assert(loc.parent() == this);
		safe_assert(BETWEEN(-1, child_index, sz-1));

		// if select thread and child_index is -1, then add this with index -1
		if(INSTANCEOF(this, SelectThreadNode*)) {
			current_nodes->push_back({this, -1});
		}

		for(int i = 0; i < sz; ++i) {
			if(i != child_index) {
				ExecutionTree* c = child(i);
				if(c == NULL || (ExecutionTreeManager::IS_TRANSNODE(c))) {
					safe_assert(c == NULL || !c->covered_);
					current_nodes->push_back({this, i});
				} else {
					ChildLoc cloc = {c, -1};
					c->PopulateLocations(cloc, current_nodes);
				}
			}
		}
	}
}

/*************************************************************************************/

DotNode* ExecutionTree::UpdateDotGraph(DotGraph* g) {
	DotNode* node = new DotNode("");
	g->AddNode(node);
	for(int i = 0; i < children_.size(); ++i) {
		ExecutionTree* c = child(i);
		DotNode* cn = NULL;
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
		} else {
			cn = new DotNode("NULL");
		}
		g->AddNode(cn);
		char s[8];
		snprintf(s, 8, "%d", i);
		g->AddEdge(new DotEdge(node, cn, s));
	}
	return node;
}

/*************************************************************************************/

DotGraph* ExecutionTree::CreateDotGraph() {
	DotGraph* g = new DotGraph("ExecutionTree");
	UpdateDotGraph(g);
	return g;
}

/*************************************************************************************/

void ExecutionTreeManager::SaveDotGraph(const char* filename) {
	safe_assert(filename != NULL);
	DotGraph g("ExecutionTree");
	DotNode* node = root_node_.UpdateDotGraph(&g);
	node->set_label("Root");
	g.WriteToFile(filename);
	char buff[256];
	snprintf(buff, 256, "dot -Tpng %s -o %s.png", filename, filename);
	system(buff);
	printf("Wrote dot file to %s\n", filename);
}

/*************************************************************************************/

ExecutionTreeManager::ExecutionTreeManager() {
	root_node_.InitChildren(1);
	lock_node_.InitChildren(0);

	Restart();
}

/*************************************************************************************/

void ExecutionTreeManager::Restart() {
	current_nodes_.clear();

	replay_path_.clear();

	// clean end_node's exceptions etc
	end_node_.clear_exceptions();
//	safe_assert(end_node_.old_root() == NULL);

	// reset semaphore value to 0
	sem_ref_.Set(0);

	// sets root as the current parent, and adds it to the path
	current_node_ = {&root_node_, 0};
	root_node_.PopulateLocations(current_node_, &current_nodes_);

	SetRef(NULL);
}

/*************************************************************************************/

ExecutionTreeManager::~ExecutionTreeManager() {
	// noop for now
	// destructors of fields are called explicitly
}

/*************************************************************************************/

void ExecutionTreeManager::PopulateLocations() {
	// evaluate other nodes in current_nodes_
	// do not touch the first one, which is the current one
	for(int i = 0, sz = current_nodes_.size(); i < sz; ++i) {
		ChildLoc loc = current_nodes_[i];
		ExecutionTree* parent = loc.parent();
		safe_assert(parent != NULL); // TODO(elmas): this can be NULL?
		// populate alternate paths
		parent->PopulateLocations(loc, &current_nodes_);
	}
}

/*************************************************************************************/

// operations by clients

// only main can run this!!!
// this always sets the timeout
ExecutionTree* ExecutionTreeManager::AcquireRefEx(AcquireRefMode mode, long timeout_usec /*= -1*/) {
	safe_assert(Coroutine::Current()->IsMain());
	if(timeout_usec < 0) timeout_usec = MaxWaitTimeUSecs;
	ExecutionTree* node = AcquireRef(mode, timeout_usec);
	if(IS_ENDNODE(node)) {
		// main has not ended the execution, so this must be due to an exception by another thread
		safe_assert(current_node_.parent() == node);
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
		if(IS_LOCKNODE(node)) {
			// noop
			Thread::Yield(true);
		}
		else
		if(IS_ENDNODE(node)) {
			SetRef(node);
			return node; // indicates end of the test
		}
		else
		if(mode == EXIT_ON_LOCK) {
			lock_node_.OnLock();
			return node; //  can be null, but cannot be lock or end node
		}
		else {
			if(IS_EMPTY(node)) {
				if(mode == EXIT_ON_EMPTY) {
					lock_node_.OnLock();
					// will process this
					return NULL;
				} else {
					SetRef(NULL);
				}
			}
			else // FULL
			if(mode == EXIT_ON_FULL) {
				lock_node_.OnLock();
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
//				VLOG(2) << "Timed wait in AcquireRef";
				int result = sem_ref_.WaitTimed(timeout_usec);
				if(result == ETIMEDOUT) {
					// fire timeout (backtrack)
					TRIGGER_BACKTRACK(TIMEOUT);
				}
				safe_assert(result == PTH_SUCCESS);
			} else {
//				VLOG(2) << "Untimed wait in AcquireRef";
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

void ExecutionTreeManager::ReleaseRef(ExecutionTree* node /*= NULL*/, int child_index /*= -1*/) {
	safe_assert(IS_LOCKNODE(GetRef(memory_order_relaxed)));
	lock_node_.OnUnlock();

	if(child_index >= 0) {
		safe_assert(node != NULL);

		// put node to the path
		AddToPath(node, child_index);

		// if released node is an end node, we do not nullify atomic_ref
		SetRef(IS_ENDNODE(node) ? node : NULL);
	} else {
		// release
		SetRef(node);
	}
	sem_ref_.Signal();

	Thread::Yield();
}

/*************************************************************************************/

void ExecutionTreeManager::AddToPath(ExecutionTree* node, int child_index) {
	safe_assert(!current_node_.empty());
	safe_assert(node != NULL);

	ChildLoc loc = GetLastInPath();
	ExecutionTree* last_node = loc.get();
	if(IS_ENDNODE(node) && last_node != NULL) {
		// set old_root of end node
		if(node != last_node) { // last_node can already be node, then skip
			safe_assert(!IS_ENDNODE(last_node));
//			static_cast<EndNode*>(node)->set_old_root(last_node);
			// delete old_root
			delete last_node;
		}
	} else {
		// if node is not end node, the we are either overwriting NULL or the same value
		safe_assert(loc.check(NULL) || loc.check(node));
	}
	loc.set(node); // also sets node's parent

	// put node to the path
	current_node_ = {node, child_index};

	node->PopulateLocations(current_node_, &current_nodes_);
}

/*************************************************************************************/

ExecutionTreePath* ExecutionTreeManager::ComputePath(ChildLoc& leaf_loc, ExecutionTreePath* path /*= NULL*/) {
	if(path == NULL) {
		path = new ExecutionTreePath();
	}
	safe_assert(path->empty());
	safe_assert(leaf_loc.parent() != NULL);

	ChildLoc loc = leaf_loc;

	ExecutionTree* p = NULL;
	while(true) {
		path->push_back(loc);
		ExecutionTree* n = loc.parent();
		ExecutionTree* p = n->parent();
		safe_assert(p != NULL || n == &root_node_);
		if(p == NULL) {
			safe_assert(n == &root_node_);
			break;
		}
		int i = p->index_of(n);
		safe_assert(i >= 0);
		loc = {p, i};
	}

	return path;
}

/*************************************************************************************/

bool ExecutionTreeManager::DoBacktrack(ChildLoc& loc, BacktrackReason reason /*= SUCCESS*/) {
	safe_assert(reason != SEARCH_ENDS && reason != EXCEPTION && reason != UNKNOWN);

	ExecutionTreePath path;
	ComputePath(loc, &path);
	safe_assert(CheckCompletePath(&path, loc));

	const int sz = path.size();
	safe_assert(sz > 1);

	//===========================
	// propagate coverage back in the path (skip the end node, which is already covered)
	int highest_covered_index = 0;
	for(int i = 1; i < sz; ++i) {
		ChildLoc element = path[i];
		safe_assert(!element.empty());
		safe_assert(element.check(path[i-1].parent()));
		ExecutionTree* node = element.parent();
		node->ComputeCoverage(false); //  do not recurse, only use immediate children

		if(node->covered()) {
			highest_covered_index = i;
		}
	}


	//===========================
	// after coverage computation:
	// first remove alternate paths which become covered, since we will delete covered subtrees below
	for(std::vector<ChildLoc>::iterator itr = current_nodes_.begin(); itr < current_nodes_.end(); ) {
		ChildLoc loc = *itr;
		safe_assert(loc.parent() != &end_node_);
		if(loc.parent()->covered()) {
			// delete it
			itr = current_nodes_.erase(itr);
		} else {
			// skip it
			++itr;
		}
	}


	//===========================
	// now delete the (largest) covered subtree
	ExecutionTree* end_node = path[0].parent();
	safe_assert(IS_ENDNODE(end_node));
	if(Config::DeleteCoveredSubtrees && BETWEEN(1, highest_covered_index, sz-2)) {
		ChildLoc parent_loc = path[highest_covered_index+1];
		ExecutionTree* subtree_root = parent_loc.get();
		safe_assert(subtree_root == path[highest_covered_index].parent());
		// TODO(elmas): collect exceptions in the subtree and contain them in end_node
		parent_loc.set(end_node); // shortcut parent to end_node
		safe_assert(path[1].parent() != end_node);
		safe_assert(path[1].get() == end_node);
		path[1].set(NULL); // remove link to the end node, to avoid deleting it
		// we can now delete the subtree
		delete subtree_root;
	}

	// current_node_ must be pointing to the end node
	safe_assert(current_node_.parent() == end_node && current_node_.child_index() == 0);

	// check if the root is covered
	return !root_node_.covered();
}


/*************************************************************************************/

ExecutionTreePath* ExecutionTreeManager::ComputeCurrentPath(ExecutionTreePath* path /*= NULL*/) {
	return ComputePath(current_node_, path);
}

/*************************************************************************************/

ChildLoc ExecutionTreeManager::GetLastInPath() {
	safe_assert(!current_node_.empty());
	return current_node_;
}

/*************************************************************************************/

bool ExecutionTreeManager::CheckCompletePath(ExecutionTreePath* path, ChildLoc& first) {
	safe_assert(path->size() >= 2);
	safe_assert(path->back().parent() == &root_node_);
	safe_assert((*path)[0] == first);
	safe_assert(IS_ENDNODE((*path)[0].get()));
	safe_assert(IS_ENDNODE((*path)[0].parent()));
	safe_assert(IS_ENDNODE((*path)[1].get()));
	safe_assert(!IS_ENDNODE((*path)[1].parent()));
	safe_assert((*path)[0].parent() == (*path)[0].get());

	return true;
}

/*************************************************************************************/

bool ExecutionTreeManager::EndWithSuccess(BacktrackReason& reason) {
	VLOG(2) << "Ending with success " << reason;
	// wait until the last node is consumed (or an end node is inserted)
	// we use AcquireRefEx to use a timeout to check if the last-inserted transition was consumed on time
	// in addition, if there is already an end node, AcquireRefEx throws a backtrack
	if(reason == THREADS_ALLENDED) {
		// no other threads, do not wait on empty, just force-lock it
		SetRef(&lock_node_);
		lock_node_.OnLock();
	} else {
		// try to first wait for empty, if we timeout, we remove it by waiting for full
		try {
			ExecutionTree* node = AcquireRefEx(EXIT_ON_EMPTY);
			safe_assert(IS_EMPTY(node));
		} catch(std::exception* e) {
			BacktrackException* be = ASINSTANCEOF(e, BacktrackException*);
			safe_assert(be != NULL);
			reason = be->reason();
			if(reason == TIMEOUT) {
				// this should not timeout
				try {
					ExecutionTree* node = AcquireRefEx(EXIT_ON_FULL);
					safe_assert(IS_FULL(node));
				} catch(std::exception* e) {
					BacktrackException* be = ASINSTANCEOF(e, BacktrackException*);
					safe_assert(be != NULL);
					reason = be->reason();
					safe_assert(reason != TIMEOUT);
				}
			}
		}
	}

	//================================================

	EndNode* end_node = ASINSTANCEOF(current_node_.get(), EndNode*);

	// check the last node in the path
	ChildLoc last = GetLastInPath();
	ExecutionTree* last_parent = last.parent();
	safe_assert(!IS_ENDNODE(last_parent));

	//===========================
	// make the parent of last element (if SelectThreadNode) covered, because there is no way to cover it
	if(reason == THREADS_ALLENDED) {
		if(INSTANCEOF(last_parent, SelectThreadNode*)) {
			last_parent->set_covered(true);
		}
	}

	//===========================
	// if last one is choice and one of the branches goes to the end of the script
	// then we make all of the instances created at the same line as covered
	if(reason == SUCCESS) {
		ChoiceNode* choice = ASINSTANCEOF(last_parent, ChoiceNode*);
		if(choice != NULL) {
			// idx goes to the end of the script
			int idx = last.child_index();
			choice->info()->set_covered(idx, true);
		}
	}

	//===========================
	ExistsThreadNode* exists = ASINSTANCEOF(last_parent, ExistsThreadNode*);
	if(exists != NULL && !exists->covered()) {
		// update covered tid set
		std::set<THREADID>* tids = exists->covered_tids();
		tids->insert(safe_notnull(exists->thread())->tid());
		// do not backtrack, just leave it there
		// this will be covered only when it cannot be consumed
		VLOG(1) << "Adding tid to covered tids";
	} else {

		// locked, create a new end node and set it
		// also adds to the path and set to ref
		if(end_node == NULL) {
			// locked, create a new end node and set it
			end_node = &end_node_; // new EndNode();
		} else {
			safe_assert(end_node == &end_node_);
		}
		// add to the path and set to ref
		AddToPath(end_node, 0);
		safe_assert(current_node_.get() == end_node);
		safe_assert(IS_ENDNODE(current_node_.get()));

		// compute coverage for the just-visited node
		if(!DoBacktrack(current_node_, reason)) {
			// root is covered, no need to continue
			safe_assert(root_node_.covered());

			safe_assert(current_node_.get() == end_node);
			safe_assert(IS_ENDNODE(current_node_.get()));
			// notify threads about the end of the controlled run
			ReleaseRef(end_node); // this does not add the node to the path, it is already added
			return false;
		}
	}

	//================================================

	// check alternate paths
	if(current_nodes_.empty()) {
		if(end_node == NULL) {
			// this means end_node has not been added to the path, so use static end_node
			safe_assert(!IS_ENDNODE(current_node_.get()))
			end_node = &end_node_;
			// update current_node to get end_node when calling GetLastInPath
			current_node_ = {end_node, 0};
		}
		// notify threads about the end of the controlled run
		ReleaseRef(end_node); // this does not add the node to the path, it is already added
//		safe_assert(IS_ENDNODE(current_node_.get()));
		return false;
	}

	//================================================

	VLOG(2) << "Alternate path exists, will replay";

	// select the next one and replay
	ChildLoc next_loc = current_nodes_.back(); current_nodes_.pop_back();

	// compute the path to follow
	replay_path_.clear();
	ComputePath(next_loc, &replay_path_);

	// remove root
	safe_assert(replay_path_.back().parent() == &root_node_);
	safe_assert(replay_path_.back().child_index() == 0);
	replay_path_.pop_back(); // remove root

	// restart for replay
	current_node_ = {&root_node_, 0};
	ReleaseRef(NULL); // nullify the atomic ref to continue

	VLOG(2) << "Starting the replay";

	return true;
}

/*************************************************************************************/

// (do not use AcquireRefEx here)
void ExecutionTreeManager::EndWithException(Coroutine* current, std::exception* exception, const std::string& where /*= "<unknown>"*/) {
	VLOG(2) << "Inserting end node to indicate exception.";
	EndNode* end_node = NULL;
	// wait until we lock the atomic_ref, but the old node can be null or any other node
	ExecutionTree* node = AcquireRef(EXIT_ON_LOCK);
	if(IS_ENDNODE(current_node_.parent())) {
		safe_assert(IS_ENDNODE(node));
		end_node = static_cast<EndNode*>(current_node_.parent());

	} else if(IS_ENDNODE(node)) {
		safe_assert(IS_ENDNODE(current_node_.get()));
		safe_assert(current_node_.get() == node);
		end_node = static_cast<EndNode*>(node);

	} else {
		ExecutionTree* last = GetLastInPath().get();
		if(IS_ENDNODE(last)) {
			end_node = static_cast<EndNode*>(last);
		} else {
			// locked, create a new end node and set it
			end_node = &end_node_; // new EndNode();
		}
		// add to the path and set to ref
		ReleaseRef(end_node, 0);
	}

	safe_assert(end_node != NULL);
	// add my exception to the end node
	end_node->add_exception(exception, current, where);
}

/*************************************************************************************/


void ExecutionTreeManager::EndWithBacktrack(Coroutine* current, BacktrackReason reason, const std::string& where) {
	EndWithException(current, GetBacktrackException(reason), where);
}


/*************************************************************************************/

TransitionConstraint::TransitionConstraint(Scenario* scenario)
: scenario_(scenario) {
	scenario_->trans_constraints()->push_back(TransitionPredicatePtr(this));
}

TransitionConstraint::~TransitionConstraint(){
	safe_assert(scenario_->trans_constraints()->back().get() == this);
	scenario_->trans_constraints()->pop_back();
}

/*************************************************************************************/

bool ForallThreadNode::ComputeCoverage(bool recurse, bool call_parent /*= false*/) {
	Scenario* scenario = Scenario::Current();
	safe_assert(scenario != NULL);
	// this check is important, because we should not compute coverage at all if already covered
	// since the computation below may turn already covered not covered
	if(!covered_) {
		covered_ = (children_.size() == scenario->group()->GetNumMembers())
					&& ExecutionTree::ComputeCoverage(recurse, call_parent);
	}
	return covered_;
}

/*************************************************************************************/

} // end namespace


