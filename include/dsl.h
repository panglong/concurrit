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


#ifndef DSL_H_
#define DSL_H_

#include "common.h"
#include "transpred.h"
#include "dot.h"

#include <cstdatomic>

namespace concurrit {

class Scenario;
class Coroutine;

/********************************************************************************/
class ChildLoc;
class ExecutionTree;
typedef std::atomic_address ExecutionTreeRef;
typedef std::vector<ExecutionTree*> ExecutionTreeList;

#define for_each_child(child) \
	ExecutionTreeList::iterator __itr__ = children_.begin(); \
	ExecutionTree* child = (__itr__ != children_.end() ? *__itr__ : NULL); \
	for (; __itr__ != children_.end(); child = ((++__itr__) != children_.end() ? *__itr__ : NULL))

class ExecutionTree : public Writable {
public:
	ExecutionTree(ExecutionTree* parent = NULL, int num_children = 0) : parent_(parent), covered_(false) {
		InitChildren(num_children);
		num_nodes_++;
	}

	virtual ~ExecutionTree();

	bool ContainsChild(ExecutionTree* node);

	void InitChildren(int n);

	ExecutionTree* child(int i = 0);
	int index_of(ExecutionTree* node);

	void add_child(ExecutionTree* node);

	void set_child(ExecutionTree* node, int i = 0);
	ExecutionTree* get_child(int i);

	virtual bool child_covered(int i = 0);
	virtual bool is_covered() { return covered_; }

	virtual bool ComputeCoverage(bool recurse, bool call_parent = false);

	virtual void ToStream(FILE* file) {
		fprintf(file, "Num children: %d, %s.", children_.size(), (covered_ ? "covered" : "not covered"));
	}

	void PopulateLocations(ChildLoc& loc, std::vector<ChildLoc>* current_nodes);

	virtual DotNode* UpdateDotGraph(DotGraph* g);
	DotGraph* CreateDotGraph();

private:
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD_REF(ExecutionTreeList, children)
	DECL_FIELD(bool, covered)

	DECL_STATIC_FIELD(int, num_nodes)

	friend class ExecutionTreeManager;
};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode(ExecutionTree* parent = NULL, int num_children = 0) : ExecutionTree(parent, num_children) {}
	virtual ~TransitionNode(){}
};

class SelectionNode : public ExecutionTree {
public:
	SelectionNode(ExecutionTree* parent = NULL, int num_children = 0) : ExecutionTree(parent, num_children) {}
	virtual ~SelectionNode(){}
};

/********************************************************************************/

class ChildLoc {
public:
	ChildLoc(ExecutionTree* parent = NULL, int child_index = -1) : parent_(parent), child_index_(child_index) {}
	~ChildLoc(){}

	void set(ExecutionTree* node) {
		safe_assert(parent_ != NULL);
		parent_->set_child(node, child_index_);
		if(node != NULL) {
			node->set_parent(parent_);
		}
	}

	ExecutionTree* get() {
		safe_assert(parent_ != NULL);
		ExecutionTree* node = parent_->get_child(child_index_);
		return node;
	}

	bool check(ExecutionTree* node) {
		safe_assert(parent_ != NULL);
		return (parent_->get_child(child_index_) == node) && (node == NULL || node->parent() == parent_);
	}

	ExecutionTree* exchange(ExecutionTree* node) {
		ExecutionTree* old = get();
		set(node);
		return old;
	}

	operator ExecutionTree* () {
		return get();
	}

	bool empty() {
		return parent_ == NULL || child_index_ < 0;
	}

	bool operator == (const ChildLoc& other) {
		return this->parent_ == other.parent_ && this->child_index_ == other.child_index_;
	}

	bool operator != (const ChildLoc& other) {
		return !(this->operator ==(other));
	}

	static ChildLoc EMPTY() {
		return {NULL, -1};
	}
private:
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD(int, child_index);
};

/********************************************************************************/

class EndNode : public ExecutionTree {
public:
	EndNode(ExecutionTree* parent = NULL) : ExecutionTree(parent, 1) {
		covered_ = true;
		set_child(this); // points to itself
		exception_ = NULL;
		old_root_ = NULL;
	}

	~EndNode(){
		// delete associated exceptions
		if(exception_ != NULL) {
			// this also clears the list of exceptions recursively (following the next_ pointer)
			delete exception_;
		}
	}

	void add_exception(std::exception* e, Coroutine* owner, const std::string& where) {
		exception_ = new ConcurritException(e, owner, where, exception_);
	}

	virtual void ToStream(FILE* file) {
		fprintf(file, "EndNode. %s exception.", (exception_ == NULL ? "Has no " : "Has "));
		ExecutionTree::ToStream(file);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("EndNode");
		g->AddNode(node);
		g->AddEdge(new DotEdge(node, node));
		if(old_root_ != NULL) {
			DotNode* cn = old_root_->UpdateDotGraph(g);
			g->AddNode(cn);
			g->AddEdge(new DotEdge(node, cn, "OLD"));
		}
		return node;
	}

private:
	DECL_FIELD(ConcurritException*, exception)
	DECL_FIELD(ExecutionTree*, old_root)
};

/********************************************************************************/

class LockNode : public ExecutionTree {
public:
	LockNode() : ExecutionTree(NULL, 0), owner_(NULL) {}
	~LockNode(){}

	void OnLock() {
		safe_assert(owner_ == NULL);
		owner_ = Coroutine::Current();
	}

	void OnUnlock() {
		safe_assert(owner_ == Coroutine::Current());
		owner_ = NULL;
	}

private:
	DECL_FIELD(Coroutine*, owner)
};

/********************************************************************************/

class StaticChoiceInfo {
public:
	StaticChoiceInfo(int line){
		line_ = line;
		covered_[0] = false;
		covered_[1] = false;
	}
	~StaticChoiceInfo(){}

	bool is_covered(int i) {
		safe_assert(i == 0 || i == 1);
		return covered_[i];
	}
	void set_covered(int i, bool covered) {
		safe_assert(i == 0 || i == 1);
		covered_[i] = covered;
	}
private:
	int line_;
	bool covered_[2];
};

class ChoiceNode : public SelectionNode {
public:
	ChoiceNode(StaticChoiceInfo* info, ExecutionTree* parent = NULL) : SelectionNode(parent, 2), info_(info) {}
	~ChoiceNode() {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "ChoiceNode.");
		ExecutionTree::ToStream(file);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("ChoiceNode");
		g->AddNode(node);
		ExecutionTree* c = child(0);
		DotNode* cn = NULL;
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
		} else {
			cn = new DotNode("NULL");
		}
		g->AddNode(cn);
		g->AddEdge(new DotEdge(node, cn, "F"));

		c = child(1);
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
		} else {
			cn = new DotNode("NULL");
		}
		g->AddNode(cn);
		g->AddEdge(new DotEdge(node, cn, "T"));

		return node;
	}

	// override
	bool child_covered(int i = 0) {
		safe_assert(i == 0 || i == 1);
		return ExecutionTree::child_covered(i) || safe_notnull(info_)->is_covered(i);
	}

	// override
	bool ComputeCoverage(bool recurse, bool call_parent = false) {
		if(!covered_) {
			safe_assert(!recurse);
			covered_ = child_covered(0) && child_covered(1);
		}
		if(covered_ && call_parent && parent_ != NULL) {
			parent_->ComputeCoverage(recurse, true);
		}
		return covered_;
	}

private:
	DECL_FIELD(StaticChoiceInfo*, info)
};

/********************************************************************************/

class SelectThreadNode : public SelectionNode {
public:
	typedef std::map<THREADID, int> TidToIdxMap;
	SelectThreadNode(const ThreadVarPtr& var, ExecutionTree* parent = NULL, int num_children = 0)
	: SelectionNode(parent, num_children), var_(var) {
		safe_assert(safe_notnull(var_.get())->thread() == NULL);
	}
	virtual ~SelectThreadNode() {}

	virtual bool is_exists() = 0;
	bool is_forall() { return !is_exists(); }

	void ToStream(FILE* file) {
		fprintf(file, "%sThreadNode. Selected %s.",
				(is_exists() ? "Exists" : "Forall"),
				((var_ != NULL && var_->thread() != NULL) ? to_string(var_->thread()->tid()).c_str() : "no thread"));
		ExecutionTree::ToStream(file);
	}

private:
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

class ExistsThreadNode : public SelectThreadNode {
public:
	ExistsThreadNode(const ThreadVarPtr& var, ExecutionTree* parent = NULL)
	: SelectThreadNode(var, parent, 1), thread_(NULL) {}

	~ExistsThreadNode() {}

	bool is_exists() { return true; }

	ChildLoc set_selected_thread() {
		safe_assert(thread_ != NULL);
		return set_selected_thread(thread_);
	}

	ChildLoc set_selected_thread(Coroutine* thread) {
		ChildLoc newnode = {this, 0};
		// set thread of the variable
		safe_assert(var_ != NULL);
		safe_assert(var_->thread() == NULL);
		thread_ = thread;
		var_->set_thread(thread);
		return newnode;
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("ExistsThreadNode");
		g->AddNode(node);
		ExecutionTree* c = child(0);
		DotNode* cn = NULL;
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
		} else {
			cn = new DotNode("NULL");
		}
		g->AddNode(cn);
		g->AddEdge(new DotEdge(node, cn, thread_ == NULL ? "-1" : to_string(thread_->tid())));

		return node;
	}

private:
	DECL_FIELD(Coroutine*, thread)
};

/********************************************************************************/

class ForallThreadNode : public SelectThreadNode {
public:
	ForallThreadNode(const ThreadVarPtr& var, ExecutionTree* parent = NULL)
	: SelectThreadNode(var, parent, 0) {}

	~ForallThreadNode() {}

	bool is_exists() { return false; }

	// override
	bool ComputeCoverage(bool recurse, bool call_parent = false);

	ChildLoc set_selected_thread(Coroutine* co) {
		int child_index = add_or_get_thread(co);
		// update newnode to point to the proper child index of select thread
		ChildLoc newnode = {this, child_index};
		safe_assert(!newnode.empty());
		// set thread of the variable
		safe_assert(var_ != NULL);
		safe_assert(var_->thread() == NULL);
		var_->set_thread(co);
		return newnode;
	}

	ChildLoc set_selected_thread(int child_index) {
		safe_assert(BETWEEN(0, child_index, idxToThreadMap_.size()-1));
		Coroutine* co = idxToThreadMap_[child_index];
		// update newnode to point to the proper child index of select thread
		ChildLoc newnode = {this, child_index};
		safe_assert(!newnode.empty());
		// set thread of the variable
		safe_assert(var_ != NULL);
		safe_assert(var_->thread() == NULL);
		var_->set_thread(co);
		return newnode;
	}

	ExecutionTree* child_by_tid(THREADID tid) {
		int index = child_index_by_tid(tid);
		return index < 0 ? NULL : child(index);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("ForallThreadNode");
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
			g->AddEdge(new DotEdge(node, cn, to_string(safe_notnull(idxToThreadMap_[i])->tid())));
		}

		return node;
	}

private:

	// returns the index of the new child
	int add_or_get_thread(Coroutine* co) {
		THREADID tid = co->tid();
		int child_index = child_index_by_tid(tid);
		if(child_index < 0) {
			const size_t sz = children_.size();
			safe_assert(idxToThreadMap_.size() == sz);
			tidToIdxMap_[tid] = sz;
			children_.push_back(NULL);
			idxToThreadMap_.push_back(co);
			return sz;
		} else {
			return child_index;
		}
	}

	int child_index_by_tid(THREADID tid) {
		TidToIdxMap::iterator itr = tidToIdxMap_.find(tid);
		if(itr == tidToIdxMap_.end()) {
			return -1;
		} else {
			return itr->second;
		}
	}

private:
	DECL_FIELD_REF(TidToIdxMap, tidToIdxMap)
	DECL_FIELD_REF(std::vector<Coroutine*>, idxToThreadMap)
};

/********************************************************************************/

class SingleTransitionNode : public TransitionNode {
public:
	SingleTransitionNode(const TransitionPredicatePtr& pred, ThreadVarPtr var = boost::shared_ptr<ThreadVar>(), ExecutionTree* parent = NULL)
	: TransitionNode(parent, 1), pred_(pred), var_(var), thread_(NULL) {}

	~SingleTransitionNode() {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "TransitionNode.");
		ExecutionTree::ToStream(file);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("TransitionNode");
		g->AddNode(node);
		ExecutionTree* c = child();
		DotNode* cn = NULL;
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
		} else {
			cn = new DotNode("NULL");
		}
		g->AddNode(cn);
		g->AddEdge(new DotEdge(node, cn, thread_ == NULL ? "?" : to_string(thread_->tid())));
		return node;
	}

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD(Coroutine*, thread) // keep performing thread here
};

/********************************************************************************/

class MultiTransitionNode : public TransitionNode {
public:
	MultiTransitionNode(ExecutionTree* parent = NULL, int num_children = 1)
	: TransitionNode(parent, num_children) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "MultiTransitionNode.");
		ExecutionTree::ToStream(file);
	}
};

/********************************************************************************/

class TransferUntilNode : public MultiTransitionNode {
public:
	TransferUntilNode(const ThreadVarPtr& var, const TransitionPredicatePtr& pred, ExecutionTree* parent = NULL)
	: MultiTransitionNode(parent, 1), var_(var), pred_(pred) {}

	~TransferUntilNode() {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "TransferUntilNode.");
		ExecutionTree::ToStream(file);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("TransferUntilNode");
		g->AddNode(node);
		ExecutionTree* c = child();
		DotNode* cn = NULL;
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
		} else {
			cn = new DotNode("NULL");
		}
		g->AddNode(cn);
		g->AddEdge(new DotEdge(node, cn, to_string(safe_notnull(var_->thread())->tid())));
		return node;
	}
private:
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/

typedef std::vector<ChildLoc> ExecutionTreePath;

enum AcquireRefMode { EXIT_ON_EMPTY = 1, EXIT_ON_FULL = 2, EXIT_ON_LOCK = 3};

class ExecutionTreeManager {
public:
	ExecutionTreeManager();
	~ExecutionTreeManager();

	void Restart();

static inline bool IS_EMPTY(ExecutionTree* n) { return ((n) == NULL); }
inline bool IS_LOCKNODE(ExecutionTree* n) { return ((n) == (&lock_node_)); }
static inline bool IS_ENDNODE(ExecutionTree* n) { return (INSTANCEOF(n, EndNode*)); }
static inline bool IS_TRANSNODE(ExecutionTree* n) { return (INSTANCEOF(n, TransitionNode*)); }
static inline bool IS_MULTITRANSNODE(ExecutionTree* n) { return (INSTANCEOF(n, MultiTransitionNode*)); }
static inline bool IS_SELECTNODE(ExecutionTree* n) { return (INSTANCEOF(n, SelectionNode*)); }

	// set atomic_ref to lock_node, and return the previous node according to mode
	// if atomic_ref is end_node, returns it immediatelly
	ExecutionTree* AcquireRef(AcquireRefMode mode, long timeout_usec = -1);
	// same as AcquireRefEx, but triggers backtrack exception if the node is an end node
	ExecutionTree* AcquireRefEx(AcquireRefMode mode, long timeout_usec = -1);

	// if child_index >= 0, adds the (node,child_index) to path and nullifies atomic_ref
	// otherwise, puts node back to atomic_ref
	void ReleaseRef(ExecutionTree* node = NULL, int child_index = -1);

	void ReleaseRef(ChildLoc& node) {
		ReleaseRef(node.parent(), node.child_index());
	}

	ChildLoc GetLastInPath();
	void AddToPath(ExecutionTree* node, int child_index);
	ExecutionTreePath* ComputePath(ChildLoc& leaf_loc, ExecutionTreePath* path = NULL);
	ExecutionTreePath* ComputeCurrentPath(ExecutionTreePath* path = NULL);
	bool DoBacktrack(ChildLoc& loc, BacktrackReason reason = SUCCESS);

	bool EndWithSuccess(BacktrackReason reason = SUCCESS);
	void EndWithException(Coroutine* current, std::exception* exception, const std::string& where = "<unknown>");
	void EndWithBacktrack(Coroutine* current, BacktrackReason reason, const std::string& where);

	bool CheckCompletePath(ExecutionTreePath* path, ChildLoc& first);

	void PopulateLocations();

	void SaveDotGraph(const char* filename);

private:
	ExecutionTree* GetRef(std::memory_order mo = memory_order_seq_cst) {
		return static_cast<ExecutionTree*>(atomic_ref_.load(mo));
	}

	ExecutionTree* ExchangeRef(ExecutionTree* node, std::memory_order mo = memory_order_seq_cst) {
		return static_cast<ExecutionTree*>(atomic_ref_.exchange(node, mo));
	}

	// normally end node cannot be overwritten, unless overwrite_end flag is set
	void SetRef(ExecutionTree* node, std::memory_order mo = memory_order_seq_cst) {
		atomic_ref_.store(node, mo);
	}

private:
	DECL_FIELD_REF(ExecutionTree, root_node)
	DECL_FIELD_REF(LockNode, lock_node)
	DECL_FIELD_REF(ChildLoc, current_node)
	DECL_FIELD_REF(std::vector<ChildLoc>, current_nodes)
	DECL_FIELD(unsigned, num_paths)

	ExecutionTreeRef atomic_ref_;
	Semaphore sem_ref_;

	DECL_FIELD(Mutex, mutex)
	DECL_FIELD(ConditionVar, cv)

	DECL_FIELD_REF(ExecutionTreePath, replay_path)

	DISALLOW_COPY_AND_ASSIGN(ExecutionTreeManager)
};

/********************************************************************************/

} // namespace


#endif /* DSL_H_ */
