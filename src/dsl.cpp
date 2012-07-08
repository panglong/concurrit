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

//TPVALUE TPNOT(TPVALUE v) {
//	static TPVALUE __not_table__ [3] = {
//			TPTRUE, TPFALSE, TPUNKNOWN
//	};
//	safe_assert(v != TPINVALID);
//	return __not_table__[v];
//}
//
//TPVALUE TPAND(TPVALUE v1, TPVALUE v2) {
//	static TPVALUE __and_table__ [3][3] = {
//	//		F			T			U
//	/*F*/	{TPFALSE,   TPFALSE, 	TPFALSE},
//	/*T*/	{TPFALSE,   TPTRUE, 	TPUNKNOWN},
//	/*U*/	{TPFALSE, 	TPUNKNOWN, 	TPUNKNOWN}
//	};
//	safe_assert(v1 != TPINVALID && v2 != TPINVALID);
//	return __and_table__[v1][v2];
//}
//
//TPVALUE TPOR(TPVALUE v1, TPVALUE v2) {
//	static TPVALUE __or_table__ [3][3] = {
//	//		F			T			U
//	/*F*/	{TPFALSE,   TPTRUE,		TPUNKNOWN},
//	/*T*/	{TPTRUE,   	TPTRUE, 	TPTRUE},
//	/*U*/	{TPUNKNOWN,	TPTRUE,		TPUNKNOWN}
//	};
//	safe_assert(v1 != TPINVALID && v2 != TPINVALID);
//	return __or_table__[v1][v2];
//}

/*************************************************************************************/

ExecutionTree::ExecutionTree(StaticDSLInfo* static_info /*= NULL*/, ExecutionTree* parent /*= NULL*/, int num_children /*= 0*/)
: static_info_(static_info), parent_(parent), covered_(false) {
	safe_assert(static_info_ != NULL);
	InitChildren(num_children);

	Scenario* scenario = Scenario::Current();
	if(scenario != NULL) scenario->counter("Num execution-tree nodes").increment(1);
}

/*************************************************************************************/

ExecutionTree::~ExecutionTree(){
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
	safe_assert(BETWEEN(0, i, int(children_.size())-1));
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
	safe_assert(BETWEEN(0, i, int(children_.size())-1));
	children_[i] = node;
}

/*************************************************************************************/

ExecutionTree* ExecutionTree::get_child(int i) {
	safe_assert(BETWEEN(0, i, int(children_.size())-1));
	return children_[i];
}

/*************************************************************************************/

bool ExecutionTree::ComputeCoverage(bool call_parent /*= false*/) {
	if(!covered_) {
		bool cov = true;
		for_each_child(child) {
			if(child != NULL) {
				cov = cov && child->covered_;
				if(!cov) break;
			} else {
				cov = false; // NULL child means undiscovered-yet branch
				break;
			}
		}
		covered_ = cov;
	}
	if(covered_ && call_parent && parent_ != NULL) {
		parent_->ComputeCoverage(/*call_parent=*/ true);
	}
	return covered_;
}

/*************************************************************************************/

//void ExecutionTree::PopulateLocations(int child_index, std::vector<ChildLoc>* current_nodes) {
//	if(ExecutionTreeManager::IS_SELECTNODE(this) && !this->covered()) {
//		int sz = children_.size();
//		safe_assert(BETWEEN(-1, child_index, sz-1));
//
//		// if select thread and child_index is -1, then add this with index -1
//		SelectThreadNode* select = ASINSTANCEOF(this, SelectThreadNode*);
//		if(select != NULL) {
//			current_nodes->push_back({this, -1});
//		}
//
//		for(int i = 0; i < sz; ++i) {
//			if(i != child_index) {
//				// check if we can proceed
//				if(select != NULL && !select->CanSelectThread(i)) {
//					continue;
//				}
//
//				ExecutionTree* c = child(i);
//				if(c == NULL || (ExecutionTreeManager::IS_TRANSNODE(c) && !c->covered())) {
//					current_nodes->push_back({this, i});
//				} else {
//					c->PopulateLocations(-1, current_nodes);
//				}
//			}
//		}
//	}
//}

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

void ExecutionTree::OnConsumed(Coroutine* current, int child_index /*= 0*/) {
	safe_assert(current != NULL);
	safe_assert(BETWEEN(0, child_index, children_.size()-1));

	if(static_info_->message() != "") {
		MYLOG(1) << "Consumed: [TID: " << safe_notnull(current)->tid() << "]" << " [ACTION: " << static_info_->message() << "]";
	}

	if(Config::SaveExecutionTraceToFile) {
		SourceLocation* srcloc = current->srcloc();
		FILE* trace_file = safe_notnull(Scenario::trace_file());
		safe_assert(strlen(current->instr_callback_info()) < 256);
		fprintf(trace_file, "C TID:%2d: [%s] -- [%s] -- [%s]\n",
				current->tid(), static_info_->message().c_str(),
				(srcloc == NULL ? "<unknown>" : srcloc->ToString().c_str()),
				current->instr_callback_info());
		fflush(trace_file);
	}
}

/*************************************************************************************/

void TransitionNode::OnConsumed(Coroutine* current, int child_index /*= 0*/) {
	ExecutionTree::OnConsumed(current, child_index);

	// update var if not null
	if(var_ != NULL) {
		var_->set_thread(current);
	}
}

/*************************************************************************************/

void TransitionNode::OnTaken(Coroutine* current, int child_index /*= 0*/) {
	safe_assert(current != NULL);
	safe_assert(BETWEEN(0, child_index, children_.size()-1));

	// update var if not null
	if(var_ != NULL) {
		var_->set_thread(current);
	}

	// update counter
	Scenario::NotNullCurrent()->counter("Num Events").increment();

	if(static_info_->message() != "") {
		MYLOG(2) << "Taken: [TID: " << safe_notnull(current)->tid() << "]" << " [ACTION: " << static_info_->message() << "]";
	}

	if(Config::SaveExecutionTraceToFile) {
		SourceLocation* srcloc = current->srcloc();
		FILE* trace_file = safe_notnull(Scenario::trace_file());
		safe_assert(strlen(current->instr_callback_info()) < 256);
		fprintf(trace_file, "T TID:%2d: [%s] -- [%s] -- [%s]\n",
				current->tid(), static_info_->message().c_str(),
				(srcloc == NULL ? "<unknown>" : srcloc->ToString().c_str()),
				current->instr_callback_info());
		fflush(trace_file);
	}
}

/*************************************************************************************/

void ExecutionTreeManager::SaveDotGraph(const char* filename) {
	safe_assert(filename != NULL);
	DotGraph g("ExecutionTree");
	DotNode* node = ROOTNODE()->UpdateDotGraph(&g);
	node->set_label("Root");
	g.WriteToFile(filename);
	char buff[256];
	snprintf(buff, 256, "dot -Tpng %s -o %s.png", filename, filename);
	system(buff);
	printf("Wrote dot file to %s\n", filename);
}

/*************************************************************************************/

ExecutionTreeManager::ExecutionTreeManager() {
	stack_index_ = 0;
	safe_assert(node_stack_.empty());
	node_stack_.push_back({ROOTNODE(), 0}); // of root node

	Restart();
}

/*************************************************************************************/

void ExecutionTreeManager::Restart() {
//	current_nodes_.clear();

//	replay_path_.clear();

	// clean end_node's exceptions etc
	ENDNODE()->clear_exceptions();
//	safe_assert(ENDNODE()->old_root() == NULL);

	// reset semaphore value to 0
	sem_ref_.Set(0);

	RestartChildIndexStack();

//	if(Config::TrackAlternatePaths) {
//		ROOTNODE()->PopulateLocations(0, &current_nodes_);
//	}

	SetRef(NULL);
}

void ExecutionTreeManager::RestartChildIndexStack() {
	if(Config::KeepExecutionTree) {
		// reset stack, since we are not relying on the stack
		node_stack_.clear();
		node_stack_.push_back({ROOTNODE(), 0}); // of root node
	}

	// sets root as the current parent, and adds it to the path
	stack_index_ = 0;
	AddToNodeStack({ROOTNODE(), 0}); // this sees the pre-inserted index 0 and makes the index 1
	safe_assert(stack_index_ == 1);
	safe_assert(!Config::KeepExecutionTree || node_stack_.size() == 1);
}

/*************************************************************************************/

ExecutionTreeManager::~ExecutionTreeManager() {
	// noop for now
	// destructors of fields are called explicitly
}

/*************************************************************************************/

// operations by clients

// only main can run this!!!
// this always sets the timeout
ExecutionTree* ExecutionTreeManager::AcquireRefEx(AcquireRefMode mode, long timeout_usec /*= -1*/) {
	safe_assert(Coroutine::Current()->IsMain());
	if(timeout_usec < 0) timeout_usec = Config::MaxWaitTimeUSecs;
	ExecutionTree* node = AcquireRef(mode, timeout_usec);
	if(IS_ENDNODE(node)) {
		// main has not ended the execution, so this must be due to an exception by another thread
		safe_assert(GetLastNodeInStack().parent() == node);
		ConcurritException* cexc = static_cast<EndNode*>(node)->exception();
		safe_assert(cexc != NULL);
		BacktrackException* be = cexc->get_backtrack();
		if(be != NULL) {
			// if the SUT triggered a backtrack exception, then re-trigger the exception from within concurrit
			throw be;
		}
		// otherwise indicate that there is a non-backtrack exception
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
		ExecutionTree* node = ExchangeRef(LOCKNODE());
		if(IS_LOCKNODE(node)) {
			// noop
			if(LOCKNODE()->owner() == Coroutine::Current()) {
				if(mode == EXIT_ON_LOCK) {
					// calling thread has the lock, so safe to return
					return node;
				} else {
					safe_fail("Double-locking of atomic_ref!");
				}
			}
			Thread::Yield(true);
		}
		else
		if(IS_ENDNODE(node)) {
			SetRef(node);
			return node; // indicates end of the test
		}
		else
		if(mode == EXIT_ON_LOCK) {
			LOCKNODE()->OnLock();
			return node; //  can be null, but cannot be lock or end node
		}
		else {
			if(IS_EMPTY(node)) {
				if(mode == EXIT_ON_EMPTY) {
					LOCKNODE()->OnLock();
					// will process this
					return NULL;
				} else {
					SetRef(NULL);
				}
			}
			else // FULL
			if(mode == EXIT_ON_FULL) {
				LOCKNODE()->OnLock();
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
				int result = sem_ref_.WaitTimed(timeout_usec);
				if(result == ETIMEDOUT) {
					// fire timeout (backtrack)
					ExecutionTree* cn = GetRef();
					MYLOG(2) << "AcquireRef: Node not consumed on time: " << (cn == NULL ? "NULL" : cn->message());
					TRIGGER_BACKTRACK(TIMEOUT);
				}
				safe_assert(result == PTH_SUCCESS);
			} else {
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
				ExecutionTree* cn = GetRef();
				MYLOG(2) << "AcquireRef: Node not consumed on time: " << (cn == NULL ? "NULL" : cn->message());
				TRIGGER_BACKTRACK(TIMEOUT);
			}
		}
	}
	unreachable();
	return NULL;
}

/*************************************************************************************/

void ExecutionTreeManager::ReleaseRef(ExecutionTree* node /*= NULL*/, int child_index /*= -1*/) {
	safe_assert(IS_LOCKNODE(GetRef(memory_order_relaxed)));
	LOCKNODE()->OnUnlock();

	// TODO(elmas): optimize (do not check for every releaseref
	const bool is_endnode = IS_ENDNODE(node);
	if(is_endnode) {
		// record stack size
		Scenario::NotNullCurrent()->avg_counter("Stack size").increment(node_stack_.size());
	}

	if(child_index >= 0) {
		safe_assert(node != NULL);

		// put node to the path
		AddToPath(node, child_index);

		// if released node is an end node, we do not nullify atomic_ref
		SetRef(is_endnode ? node : NULL);
	} else {
		// release
		SetRef(node);
	}
	sem_ref_.Signal();

	Thread::Yield();
}

/*************************************************************************************/

void ExecutionTreeManager::AddToPath(ExecutionTree* node, int child_index) {
	safe_assert(node != NULL && child_index >= 0);

	ChildLoc loc = GetLastNodeInStack();
	safe_assert(!loc.empty());
	if(IS_ENDNODE(node)) {
		ExecutionTree* last_node = loc.get();
		safe_assert(last_node != NULL || stack_index_ == node_stack_.size());
		if(last_node != NULL) {
			// truncate the path since we are adding end node in the middle
			const int sz = node_stack_.size();
			safe_assert(BETWEEN(0, stack_index_, sz));
			if(stack_index_ < sz) {
				TruncateNodeStack(stack_index_+1);
				safe_assert(stack_index_ == node_stack_.size());
				safe_assert(!node_stack_.empty() && !node_stack_.back().empty());
				safe_assert(loc.get() == node_stack_.back().parent());
//				current_node_ =
				loc = GetLastNodeInStack();
				safe_assert(!loc.empty());
				last_node = loc.get();
			}
			// set old_root of end node
			if(node != last_node) { // last_node can already be node, then skip
				safe_assert(!IS_ENDNODE(last_node));
				// delete old_root
				delete last_node;
			}
		}
	} else {
		// if node is not end node, the we are either overwriting NULL or the same value
		safe_assert(loc.check(NULL) || loc.check(node));
	}

	safe_assert(!loc.empty());
	loc.set(node); // also sets node's parent

	// put node to the path
	AddToNodeStack({node, child_index});

//	if(Config::TrackAlternatePaths) {
//		node->PopulateLocations(child_index, &current_nodes_);
//	}
}

/*************************************************************************************/

// use stack_element if not -1
void ExecutionTreeManager::AddToNodeStack(const ChildLoc& current) {
	safe_assert(!current.empty());
	const int sz = node_stack_.size();

	// if we are adding end node, then the stack must have been truncated
	safe_check(!IS_ENDNODE(current.parent()) || stack_index_ == sz);

	if(stack_index_ == sz) {
		node_stack_.push_back(current);
	} else {
		safe_check(BETWEEN(0, stack_index_, (sz-1)) && node_stack_[stack_index_] == current);
	}
	++stack_index_;
//	current_node_ = current;
}

/*************************************************************************************/

ChildLoc ExecutionTreeManager::GetNextNodeInStack() {
	safe_assert(BETWEEN(0, stack_index_, node_stack_.size()));
	if(stack_index_ < node_stack_.size()) {
		ChildLoc loc = node_stack_[stack_index_];
		safe_assert(!loc.empty());
		return loc;
	} else {
		return {node_stack_.back().get(), -1};
	}
}

/*************************************************************************************/

ChildLoc ExecutionTreeManager::GetLastNodeInStack() {
	const int index = stack_index_ - 1;
	const int sz = node_stack_.size();
	safe_assert(BETWEEN(0, index, sz-1));
	ChildLoc loc = (index == (sz-1)) ? node_stack_.back() : node_stack_[index];
	safe_assert(!loc.empty());
	return loc;
}

/*************************************************************************************/

void ExecutionTreeManager::TruncateNodeStack(int index) {
	safe_assert(BETWEEN(0, index, node_stack_.size()));

	if(index < node_stack_.size()) {
		for(ExecutionTreePath::iterator itr = node_stack_.begin()+index; itr < node_stack_.end(); ) {
			itr = node_stack_.erase(itr);
		}
	}
	safe_assert(index == node_stack_.size());
	stack_index_ = index;
}

/*************************************************************************************/

ExecutionTreePath* ExecutionTreeManager::ComputePath(ChildLoc loc, ExecutionTreePath* path /*= NULL*/) {
	if(path == NULL) {
		path = new ExecutionTreePath();
	}
	safe_assert(path->empty());
	safe_assert(loc.parent() != NULL);

	ExecutionTree* p = NULL;
	while(true) {
		path->push_back(loc);
		ExecutionTree* n = loc.parent();
		ExecutionTree* p = n->parent();
		safe_assert(p != NULL || n == ROOTNODE());
		if(p == NULL) {
			safe_assert(n == ROOTNODE());
			break;
		}
		int i = p->index_of(n);
		safe_assert(i >= 0);
		loc = {p, i};
	}

	return path;
}

/*************************************************************************************/

bool ExecutionTreeManager::DoBacktrack(BacktrackReason reason /*= SUCCESS*/) throw() {
	safe_assert(reason != SEARCH_ENDS && reason != EXCEPTION && reason != UNKNOWN);

//	ExecutionTreePath path;
//	ComputePath(loc, &path);
	safe_assert(CheckCompletePath(&node_stack_));

	const int sz = node_stack_.size();
	safe_assert(sz > 1);

	//===========================
	// propagate coverage back in the path (skip the end node, which is already covered)
	int highest_covered_index = sz-1;
	for(ExecutionTreePath::reverse_iterator itr = node_stack_.rbegin()+1; itr < node_stack_.rend(); ++itr) {
		ChildLoc& element = (*itr); // node_stack_[i];
		safe_assert(!element.empty());
		safe_assert(element.check(node_stack_[highest_covered_index].parent()));
		ExecutionTree* node = element.parent();
		if(!node->ComputeCoverage()) { //  do not recurse, only use immediate children
			break;
		}
		--highest_covered_index;
	}
	safe_assert(BETWEEN(0, highest_covered_index, sz-1));
	safe_assert(node_stack_[highest_covered_index].parent()->covered());
	safe_assert(highest_covered_index == 0 || !node_stack_[highest_covered_index-1].parent()->covered());

	//===========================
	// after coverage computation:
	// first remove alternate paths which become covered, since we will delete covered subtrees below
//	safe_assert(Config::TrackAlternatePaths || current_nodes_.empty());
//	if(Config::TrackAlternatePaths) {
//		for(std::vector<ChildLoc>::iterator itr = current_nodes_.begin(); itr < current_nodes_.end(); ) {
//			ChildLoc loc = *itr;
//			safe_assert(!IS_ENDNODE(loc.parent()));
//			if(loc.parent()->covered()) {
//				// delete it
//				itr = current_nodes_.erase(itr);
//			} else {
//				// skip it
//				++itr;
//			}
//		}
//	}

	//===========================
	// now delete the (largest) covered subtree
	safe_assert(IS_ENDNODE(node_stack_.back().parent()));
	safe_assert(IS_ENDNODE(node_stack_.back().get()));
	safe_assert(highest_covered_index > 1 || ROOTNODE()->covered()); // sanity check
	if((Config::DeleteCoveredSubtrees) && BETWEEN(1, highest_covered_index, sz-2)) {
		ChildLoc parent_loc = node_stack_[highest_covered_index-1];
		ExecutionTree* subtree_root = parent_loc.get();
		safe_assert(subtree_root == node_stack_[highest_covered_index].parent());
		// TODO(elmas): collect exceptions in the subtree and contain them in end_node
		parent_loc.set(ENDNODE()); // shortcut parent to end_node
		safe_assert(node_stack_[sz-2].parent() != ENDNODE());
		safe_assert(node_stack_[sz-2].get() == ENDNODE());
		node_stack_[sz-2].set(NULL); // remove link to the end node, to avoid deleting it
		// we can now delete the subtree
		delete subtree_root;
	}

	//===========================
	// truncate the stack
	safe_assert(BETWEEN(0, highest_covered_index, (sz-1)));
	TruncateNodeStack(highest_covered_index > 0 ? highest_covered_index-1 : 0);

	//===========================
	// check if the root is covered
	return !ROOTNODE()->covered();
}


/*************************************************************************************/

bool ExecutionTreeManager::CheckCompletePath(ExecutionTreePath* path) {
	const int sz = path->size();
	safe_assert(sz >= 2);
	safe_assert((*path)[0].parent() == ROOTNODE());
//	safe_assert(path->back() == first);
	safe_assert(IS_ENDNODE(path->back().get()));
	safe_assert(IS_ENDNODE(path->back().parent()));
	safe_assert(IS_ENDNODE((*path)[sz-2].get()));
	safe_assert(!IS_ENDNODE((*path)[sz-2].parent()));
	safe_assert(path->back().parent() == path->back().get());

	return true;
}

/*************************************************************************************/

bool ExecutionTreeManager::EndWithSuccess(BacktrackReason* reason) throw() {
	MYLOG(2) << "Ending with success " << reason;

	// wait until the last node is consumed (or an end node is inserted)
	// we use AcquireRefEx to use a timeout to check if the last-inserted transition was consumed on time
	// in addition, if there is already an end node, AcquireRefEx throws a backtrack
	if(*reason == THREADS_ALLENDED) {
		// no other threads, do not wait on empty, just force-lock it
		SetRef(LOCKNODE());
		LOCKNODE()->OnLock();
	} else {
		// try to remove the current ref by waiting for empty or full
		try {
			MYLOG(2) << "AcquireRefEx in EndWithSuccess.";
			ExecutionTree* node = AcquireRefEx(EXIT_ON_LOCK);
			safe_assert(IS_EMPTY(node) || IS_FULL(node));
		} catch(std::exception* e) {
			BacktrackException* be = ASINSTANCEOF(e, BacktrackException*);
			safe_assert(be != NULL);
			*reason = be->reason();
			safe_assert(*reason == EXCEPTION);
			safe_assert(IS_ENDNODE(GetRef()));
			return false;
//			// reason may be EXCEPTION
//			if(*reason == EXCEPTION) {
//				// should terminate the search
//				safe_assert(IS_ENDNODE(GetRef()));
//				return false;
//			}
//			else if(*reason == TIMEOUT) {
//				// this should not timeout
//				try {
//					MYLOG(2) << "AcquireRefEx-2 in EndWithSuccess.";
//					ExecutionTree* node = AcquireRefEx(EXIT_ON_LOCK);
//					safe_assert(IS_FULL(node));
//				} catch(std::exception* e) {
//					BacktrackException* be = ASINSTANCEOF(e, BacktrackException*);
//					safe_assert(be != NULL);
//					*reason = be->reason();
//					safe_assert(*reason == EXCEPTION);
//					safe_assert(IS_ENDNODE(GetRef()));
//					return false;
//				}
//			}
		}
	}
	safe_assert(IS_LOCKNODE(GetRef()) || IS_ENDNODE(GetRef()));

	//================================================

	// check the last node in the path
	ChildLoc last = GetLastNodeInStack();
	safe_assert(!last.empty());
	ExecutionTree* last_parent = last.parent();

//	EndNode* end_node = ASINSTANCEOF(last.get(), EndNode*);

	//===========================
	// make the parent of last element (if SelectThreadNode) covered, because there is no way to cover it
	if(*reason == THREADS_ALLENDED) {
		if(INSTANCEOF(last_parent, SelectThreadNode*)) {
			last_parent->set_covered(true);
		}
	}

	//===========================
	// if last one is choice and one of the branches goes to the end of the script
	// then we make all of the instances created at the same line as covered
	if(*reason == SUCCESS) {
		if(Config::MarkEndingBranchesCovered) {
			ChoiceNode* choice = ASINSTANCEOF(last_parent, ChoiceNode*);
			if(choice != NULL) {
				// idx goes to the end of the script
				int idx = last.child_index();
				safe_notnull(ASINSTANCEOF(choice->static_info(), StaticChoiceInfo*))->set_covered(idx, true);
			}
		}
	}

	//===========================
//	bool backtracked = false;
	ExistsThreadNode* exists = ASINSTANCEOF(last_parent, ExistsThreadNode*);
	if(exists != NULL && !exists->covered()) {
		// update covered var set
		exists->UpdateCoveredVars();
		// do not backtrack, just leave it there
		// this will be covered only when it cannot be consumed
		MYLOG(2) << "Adding tid to covered tids";
	} else {

		// locked, create a new end node and set it
		// also adds to the path and set to ref
//		if(end_node == NULL) {
//			// locked, create a new end node and set it
//			end_node = ENDNODE(); // new EndNode();
//		} else {
//			safe_assert(end_node == ENDNODE());
//		}
		// add to the path and set to ref
		if(!IS_ENDNODE(last_parent)) {
			AddToPath(ENDNODE(), 0);
		}
		safe_assert(IS_ENDNODE(GetLastNodeInStack().get()));

		// compute coverage for the just-visited node
//		backtracked = true;
		if(!DoBacktrack(*reason)) {
			// root is covered, no need to continue
			safe_assert(ROOTNODE()->covered());

			// notify threads about the end of the controlled run
			ReleaseRef(ENDNODE()); // this does not add the node to the path, it is already added
			return false;
		}
	}

	//================================================

	// check alternate paths
//	safe_assert(Config::TrackAlternatePaths || current_nodes_.empty());
//	if(!Config::TrackAlternatePaths || !RestartForAlternatePath()) {
//		if(!backtracked) {
//			// this means dobacktrack has not been run, and end_node has not been added to the path, so use static end_node
//			safe_assert(!IS_ENDNODE(GetLastNodeInStack().get()));
//			// update current_node to get end_node when calling GetLastInPath
//			AddToNodeStack({ENDNODE(), 0});
//		}
		// notify threads about the end of the controlled run
		ReleaseRef(ENDNODE()); // this does not add the node to the path, it is already added
		return false;
//	}

//	return true;
}

/*************************************************************************************/

//bool ExecutionTreeManager::RestartForAlternatePath() {
//	safe_notnull(Scenario::Current())->counter("Num alternate paths collected").increment(current_nodes_.size());
//
//	int index_in_stack = -1;
//	ChildLoc next_loc = ChildLoc::EMPTY();
//	while(index_in_stack < 0 && !current_nodes_.empty()) {
//		// select the next one and replay
//		next_loc = current_nodes_.back(); current_nodes_.pop_back();
//
//		safe_assert(next_loc.parent() != NULL);
//		safe_assert(!next_loc.parent()->covered());
//		safe_assert(next_loc.empty() || next_loc.get() == NULL || !next_loc.get()->covered());
//
//		// truncate execution stack so that the last element is next_loc
//		index_in_stack = GetIndexInNodesStack(next_loc);
//	}
//
//	if(index_in_stack < 0) return false;
//
//	MYLOG(2) << "Alternate path exists, will replay";
//
//	safe_assert(next_loc.parent() != NULL);
//	safe_assert(!next_loc.parent()->covered());
//	safe_assert(next_loc.empty() || next_loc.get() == NULL || !next_loc.get()->covered());
//
//	// truncate stack after (including) next_loc
//	TruncateNodeStack(index_in_stack);
//
//	// compute the path to follow
//	replay_path_.clear();
//	ComputePath(next_loc, &replay_path_);
//
//	// remove root
//	ChildLoc root_loc = replay_path_.pop(); // remove root
//	safe_assert(root_loc.parent() == ROOTNODE());
//	safe_assert(root_loc.child_index() == 0);
//
//	// restart for replay
//	RestartChildIndexStack();
//
//	ReleaseRef(NULL); // nullify the atomic ref to continue
//
//	MYLOG(2) << "Starting the replay";
//
//	safe_notnull(Scenario::Current())->counter("Num alternate paths explored");
//
//	return true;
//}

/*************************************************************************************/

int ExecutionTreeManager::GetIndexInNodesStack(ChildLoc& loc) {
	const ExecutionTree* qparent = loc.parent();
	const int sz = node_stack_.size();
	safe_assert(sz > 1);

	//===========================
	// propagate coverage back in the path (skip the end node, which is already covered)
	int index = sz-1;
	for(ExecutionTreePath::reverse_iterator itr = node_stack_.rbegin(); itr < node_stack_.rend(); ++itr) {
		ChildLoc& element = (*itr);
		safe_assert(!element.empty());
		safe_assert(index == sz-1 || element.check(node_stack_[index+1].parent()));
		ExecutionTree* parent = element.parent();
		if(parent == qparent) {
			// loc must be pointing to a different child
			safe_assert(element.child_index() != loc.child_index());
			break;
		}
		--index;
	}
	safe_assert(BETWEEN(-1, index, sz-1));
	safe_assert(index < 0 || (loc.parent() == node_stack_[index].parent()));
	return index;
}

/*************************************************************************************/

void ExecutionTreeManager::EndWithBacktrack(Coroutine* current, BacktrackReason reason, const std::string& where) throw() {
	EndWithException(current, GetBacktrackException(reason), where);
}

/*************************************************************************************/

// (do not use AcquireRefEx here)
void ExecutionTreeManager::EndWithException(Coroutine* current, std::exception* exception, const std::string& where /*= "<unknown>"*/) throw() {
	MYLOG(2) << "Inserting end node to indicate exception.";
	// wait until we lock the atomic_ref, but the old node can be null or any other node
	ExecutionTree* node = NULL;
	try {
		node = AcquireRef(EXIT_ON_LOCK);
	} catch(...) {
		safe_fail("Unexpected exception (possibly due to TIMEOUT)!");
	}

	// add my exception to the end node
	// do it before releasing the node to atomic_ref
	ENDNODE()->add_exception(exception, current, where);

	if(!IS_ENDNODE(node)) {
		// add to the path and set to ref
		ReleaseRef(ENDNODE(), 0);
	}

	MYLOG(2) << "Inserted end node to indicate exception.";
}


/*************************************************************************************/

bool ForallThreadNode::ComputeCoverage(bool call_parent /*= false*/) {
	Scenario* scenario = safe_notnull(Scenario::Current());
	// this check is important, because we should not compute coverage at all if already covered
	// since the computation below may turn already covered not covered

	if(!covered_) {
		// scope_size_ == 0 means scope is NULL, so use the total number of threads when needed
		size_t sz = scope_size_ == 0 ? scenario->group()->GetNumMembers() : scope_size_;
		covered_ = (children_.size() == sz) && ExecutionTree::ComputeCoverage(call_parent);
	}
	return covered_;
}

/*************************************************************************************/

PersistentSchedule* ExecutionTreePath::ComputeExecutionTreeStack(PersistentSchedule* schedule /*= NULL*/) {
	if(schedule == NULL) {
		schedule = new PersistentSchedule();
	}
	for(iterator itr = begin(); itr < end(); ++itr) {
		ChildLoc& loc = (*itr);
		safe_assert(!loc.empty());
		ExecutionTree* parent = loc.parent();
		int child_index = loc.child_index();
		SelectThreadNode* select = ASINSTANCEOF(parent, SelectThreadNode*);
		if(select != NULL) {
			ThreadVarPtr var = select->var(child_index);
			safe_assert(var != NULL || !var->is_empty());
			schedule->push_back({ScheduleItem_ThreadId, var->tid()});
		} else {
			schedule->push_back({ScheduleItem_ChildIndex, child_index});
		}
	}
	return schedule;
}

/*************************************************************************************/

// override
void PersistentSchedule::Load(Serializer* serializer) {
	int sz;
	if(!serializer->Load<int>(&sz)) safe_fail("Error in reading schedule!\n");
	safe_assert(sz >= 0);
	if(sz > 0) {
		for(int i = 0; i < sz; ++i) {
			ScheduleItem item;
			if(!serializer->Load<ScheduleItem>(&item)) safe_fail("Error in reading schedule!\n");
			this->push_back(item);
		}
	}
	MYLOG(2) << "PersistentSchedule: Loaded " << sz << " items";
}

//override
void PersistentSchedule::Store(Serializer* serializer) {
	serializer->Store<int>(int(this->size()));
	for(iterator itr = begin(); itr < end(); ++itr) {
		serializer->Store<ScheduleItem>(*itr);
	}
	MYLOG(2) << "PersistentSchedule: Stored " << size() << " items";
}

/*************************************************************************************/


} // end namespace


