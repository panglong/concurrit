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

#include <cstdatomic>

namespace concurrit {

class Scenario;
class Coroutine;

/********************************************************************************/

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

	void add_child(ExecutionTree* node);

	void set_child(ExecutionTree* node, int i = 0);
	ExecutionTree* get_child(int i);

	bool child_covered(int i = 0);

	virtual void ComputeCoverage(Scenario* scenario, bool recurse);

	virtual void ToStream(FILE* file) {
		fprintf(file, "Num children: %d, %s.", children_.size(), (covered_ ? "covered" : "not covered"));
	}

private:
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD_REF(ExecutionTreeList, children)
	DECL_FIELD(bool, covered)

	DECL_STATIC_FIELD(int, num_nodes)

	friend class ExecutionTreeManager;
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

	void add_exception(std::exception* e, Coroutine* owner, const std::string& where) {
		exception_ = new ConcurritException(e, owner, where, exception_);
	}

	virtual void ToStream(FILE* file) {
		fprintf(file, "EndNode. %s exception.", (exception_ == NULL ? "Has no " : "Has "));
		ExecutionTree::ToStream(file);
	}

private:
	DECL_FIELD(ConcurritException*, exception)
	DECL_FIELD(ExecutionTree*, old_root)
};

/********************************************************************************/

class ChoiceNode : public ExecutionTree {
public:
	ChoiceNode(ExecutionTree* parent = NULL) : ExecutionTree(parent, 2) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "ChoiceNode.");
		ExecutionTree::ToStream(file);
	}
};

/********************************************************************************/

class SelectThreadNode : public ExecutionTree {
public:
	typedef std::map<THREADID, int> TidToIdxMap;
	SelectThreadNode(const ThreadVarPtr& var, ExecutionTree* parent = NULL)
	: ExecutionTree(parent, 0), var_(var) {
		safe_assert(var_->thread() == NULL);
	}
	~SelectThreadNode() {}

	ChildLoc set_selected_thread(Coroutine* co) {
		int child_index = add_or_get_thread(co);
		// update newnode to point to the proper child index of select thread
		ChildLoc newnode = {this, child_index};
		safe_assert(!newnode.empty());
		// set thread of the variable
		safe_assert(var_->thread() == NULL);
		var_->set_thread(co);
		return newnode;
	}

	ExecutionTree* child_by_tid(THREADID tid) {
		int index = child_index_by_tid(tid);
		return index < 0 ? NULL : child(index);
	}

	// override
	virtual void ComputeCoverage(Scenario* scenario, bool recurse);

	virtual void ToStream(FILE* file) {
		fprintf(file, "SelectThreadNode. Selected %s.", ((var_ != NULL && var_->thread() != NULL) ? var_->thread()->name().c_str() : "no thread"));
		ExecutionTree::ToStream(file);
	}

private:

	// returns the index of the new child
	int add_or_get_thread(Coroutine* co) {
		THREADID tid = co->coid();
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
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD_REF(TidToIdxMap, tidToIdxMap)
	DECL_FIELD_REF(std::vector<Coroutine*>, idxToThreadMap)

};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode(TransitionPredicate* pred, ThreadVarPtr var = boost::shared_ptr<ThreadVar>(), ExecutionTree* parent = NULL)
	: ExecutionTree(parent, 1), pred_(pred), var_(var), thread_(NULL) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "TransitionNode.");
		ExecutionTree::ToStream(file);
	}
private:
	DECL_FIELD(TransitionPredicate*, pred)
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD(Coroutine*, thread) // keep performing thread here
};

/********************************************************************************/

class TransferUntilNode : public ExecutionTree {
public:
	TransferUntilNode(const ThreadVarPtr& var, TransitionPredicate* pred, ExecutionTree* parent = NULL)
	: ExecutionTree(parent, 1), var_(var), pred_(pred) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "TransferUntilNode.");
		ExecutionTree::ToStream(file);
	}
private:
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD(TransitionPredicate*, pred)
};

/********************************************************************************/

enum AcquireRefMode { EXIT_ON_EMPTY = 1, EXIT_ON_FULL = 2, EXIT_ON_LOCK = 3};

class ExecutionTreeManager {
public:
	ExecutionTreeManager();

	void Restart();

inline bool REF_EMPTY(ExecutionTree* n) { return ((n) == NULL); }
inline bool REF_LOCKED(ExecutionTree* n) { return ((n) == (&lock_node_)); }
inline bool REF_ENDTEST(ExecutionTree* n) { return (INSTANCEOF(n, EndNode*)); }

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

	void EndWithSuccess();
	void EndWithException(Coroutine* current, std::exception* exception, const std::string& where = "<unknown>");
	void EndWithBacktrack(Coroutine* current, const std::string& where);

	bool CheckCompletePath(std::vector<ChildLoc>* path = NULL);

private:
	ExecutionTree* GetRef();
	ExecutionTree* ExchangeRef(ExecutionTree* node);
	void SetRef(ExecutionTree* node);

private:
	DECL_FIELD_REF(ExecutionTree, root_node)
	DECL_FIELD_REF(ExecutionTree, lock_node)
	DECL_FIELD_REF(std::vector<ChildLoc>, current_path)
	DECL_FIELD(unsigned, num_paths)

	ExecutionTreeRef atomic_ref_;
	Semaphore sem_ref_;

	DECL_FIELD(Mutex, mutex)
	DECL_FIELD(ConditionVar, cv)


	DISALLOW_COPY_AND_ASSIGN(ExecutionTreeManager)
};

/********************************************************************************/

} // namespace


#endif /* DSL_H_ */
