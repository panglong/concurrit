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

/********************************************************************************/

class StaticDSLInfo {
public:
	StaticDSLInfo(SourceLocation* loc = NULL, const char* message = NULL) : srcloc_(loc), message_(message == NULL ? "" : std::string(message)) {}
	virtual ~StaticDSLInfo() {
		safe_delete(srcloc_);
	}

private:
	DECL_FIELD(SourceLocation*, srcloc)
	DECL_FIELD(std::string, message)

	friend class ExecutionTree;
};

/********************************************************************************/

#define for_each_child(child) \
	ExecutionTreeList::iterator __itr__ = children_.begin(); \
	ExecutionTree* child = (__itr__ != children_.end() ? *__itr__ : NULL); \
	for (; __itr__ != children_.end(); child = ((++__itr__) != children_.end() ? *__itr__ : NULL))

class ExecutionTree : public Writable {
public:
	ExecutionTree(StaticDSLInfo* static_info = NULL, ExecutionTree* parent = NULL, int num_children = 0)
	: static_info_(static_info), parent_(parent), covered_(false) {
		safe_assert(static_info_ != NULL);
		InitChildren(num_children);

		safe_notnull(Scenario::Current())->counter("Num execution-tree nodes").increment();
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

	virtual bool ComputeCoverage(bool call_parent = false);

	virtual void ToStream(FILE* file) {
		fprintf(file, "Num children: %d, %s.", children_.size(), (covered_ ? "covered" : "not covered"));
	}

	void PopulateLocations(int child_index, std::vector<ChildLoc>* current_nodes);

	virtual DotNode* UpdateDotGraph(DotGraph* g);
	DotGraph* CreateDotGraph();

	virtual void OnConsumed(Coroutine* current, int child_index = 0);

	virtual void OnSubmitted() {
		if(static_info_->message_ != "") {
			MYLOG(1) << "Submitted: [ACTION: " << static_info_->message() << "]";
		}
	}

	std::string& message() { return static_info_->message(); }

private:
	DECL_FIELD(StaticDSLInfo*, static_info)
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD_REF(ExecutionTreeList, children)
	DECL_FIELD(bool, covered)

	friend class ExecutionTreeManager;
};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode(StaticDSLInfo* static_info,
				   const TransitionPredicatePtr& assertion,
				   const TransitionPredicatePtr& pred,
				   const ThreadVarPtr& var = ThreadVarPtr(),
				   const TransitionConstraintsPtr& constraints = TransitionConstraintsPtr(),
				   ExecutionTree* parent = NULL, int num_children = 0)
	: ExecutionTree(static_info, parent, num_children), assertion_(assertion), pred_(pred), var_(var), constraints_(constraints) {}
	virtual ~TransitionNode(){}

	virtual void OnTaken(Coroutine* current, int child_index = 0);

	void Update(const TransitionPredicatePtr& assertion,
				const TransitionPredicatePtr& pred,
			   const ThreadVarPtr& var = ThreadVarPtr(),
			   const TransitionConstraintsPtr& constraints = TransitionConstraintsPtr()) {
		assertion_ = assertion;
		pred_ = pred;
		var_ = var;
		constraints_ = constraints;
	}

private:
	DECL_FIELD(TransitionPredicatePtr, assertion)
	DECL_FIELD(TransitionPredicatePtr, pred)
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD(TransitionConstraintsPtr, constraints)
};

class SelectionNode : public ExecutionTree {
public:
	SelectionNode(StaticDSLInfo* static_info = NULL, ExecutionTree* parent = NULL, int num_children = 0)
	: ExecutionTree(static_info, parent, num_children) {}
	virtual ~SelectionNode(){}
};

/********************************************************************************/

class ChildLoc {
public:
	ChildLoc(ExecutionTree* parent = NULL, int child_index = -1) : parent_(parent), child_index_(child_index) {}
	~ChildLoc(){}

	void set(ExecutionTree* node) {
		safe_assert(!empty());
		parent_->set_child(node, child_index_);
		if(node != NULL) {
			node->set_parent(parent_);
		}
	}

	ExecutionTree* get() {
		safe_assert(!empty());
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

	bool empty() const {
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
	DECL_FIELD_CONST(ExecutionTree*, parent)
	DECL_FIELD_CONST(int, child_index);
};

/********************************************************************************/

class EndNode : public ExecutionTree {
public:
	EndNode(ExecutionTree* parent = NULL) : ExecutionTree(new StaticDSLInfo(NULL, "EndNode"), parent, 1) {
		covered_ = true;
		set_child(this); // points to itself
		exception_ = NULL;
//		old_root_ = NULL;
	}

	virtual ~EndNode(){
		clear_exceptions();
	}

	void clear_exceptions() {
		// delete associated exceptions
		if(exception_ != NULL) {
			// this also clears the list of exceptions recursively (following the next_ pointer)
			delete exception_;
			exception_ = NULL;
		}
	}

	void add_exception(std::exception* e, Coroutine* owner, const std::string& where) {
		safe_assert(e != NULL);
		exception_ = new ConcurritException(e, owner, where, exception_);
	}

	virtual void ToStream(FILE* file) {
		fprintf(file, "%s", ToString().c_str());
		ExecutionTree::ToStream(file);
	}

	std::string ToString() {
		std::stringstream s;
		s << "EndNode(";
		if(exception_ == NULL) {
			s << "SUCCESS";
		} else {
			BacktrackException* be = ASINSTANCEOF(exception_, BacktrackException*);
			if(be != NULL) {
				s << BacktrackException::ReasonToString(be->reason());
			} else {
				AssertionViolationException* ae = ASINSTANCEOF(exception_, AssertionViolationException*);
				if(ae != NULL) {
					s << "ASSERT";
				} else {
					s << exception_->what();
				}
			}
		}
		s << ")";
		return s.str();
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode(this->ToString());
		g->AddNode(node);
		g->AddEdge(new DotEdge(node, node));
//		if(old_root_ != NULL) {
//			DotNode* cn = old_root_->UpdateDotGraph(g);
//			g->AddNode(cn);
//			g->AddEdge(new DotEdge(node, cn, "OLD"));
//		}
		return node;
	}

private:
	DECL_FIELD(ConcurritException*, exception)
//	DECL_FIELD(ExecutionTree*, old_root)

	DISALLOW_COPY_AND_ASSIGN(EndNode)
};

/********************************************************************************/

class StaticEndNode : public EndNode {
public:
	StaticEndNode() : EndNode(NULL) {}

	~StaticEndNode(){
		CHECK(false) << "StaticEndNode should not be deleted!!!";
	}

	DISALLOW_COPY_AND_ASSIGN(StaticEndNode)
};

/********************************************************************************/

class LockNode : public ExecutionTree {
public:
	LockNode() : ExecutionTree(new StaticDSLInfo(NULL, "LockNode"), NULL, 0), owner_(NULL) {}
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

class RootNode : public ExecutionTree {
public:
	RootNode() : ExecutionTree(new StaticDSLInfo(NULL, "RootNode"), NULL, 1) {}
	~RootNode(){}
};

/********************************************************************************/

class StaticChoiceInfo : public StaticDSLInfo {
public:
	StaticChoiceInfo(bool nondet, SourceLocation* loc = NULL, const char* code = NULL) : StaticDSLInfo(loc, code) {
		safe_assert(loc != NULL);
		nondet_ = nondet;
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
	DECL_FIELD(bool, nondet)
	bool covered_[2];
};

/********************************************************************************/

class ChoiceNode : public SelectionNode {
public:
	ChoiceNode(StaticDSLInfo* info, ExecutionTree* parent = NULL)
	: SelectionNode(info, parent, 2) {}
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
		return ExecutionTree::child_covered(i) || safe_notnull(ASINSTANCEOF(static_info_, StaticChoiceInfo*))->is_covered(i);
	}

	// override
	bool ComputeCoverage(bool call_parent = false) {
		if(!covered_) {
			covered_ = child_covered(0) && child_covered(1);
		}
		if(covered_ && call_parent && parent_ != NULL) {
			parent_->ComputeCoverage(true);
		}
		safe_assert(!covered_ || (child_covered(0) && child_covered(1)));
		safe_assert(covered_ || (!child_covered(0) || !child_covered(1)));
		return covered_;
	}
};

/********************************************************************************/

class SelectThreadNode : public SelectionNode {
public:
	typedef std::map<THREADID, int> TidToIdxMap;
	SelectThreadNode(StaticDSLInfo* static_info = NULL,
					 const TransitionPredicatePtr& pred = TransitionPredicatePtr(),
					 ExecutionTree* parent = NULL, int num_children = 0)
	: SelectionNode(static_info, parent, num_children), pred_(pred) {}
	virtual ~SelectThreadNode() {}

	virtual bool is_exists() = 0;
	bool is_forall() { return !is_exists(); }

	void ToStream(FILE* file) {
		fprintf(file, "%sThreadNode.", (is_exists() ? "Exists" : "Forall"));
		ExecutionTree::ToStream(file);
	}

	void Update(const TransitionPredicatePtr& pred) {
		pred_ = pred;
	}

	bool CanSelectThread(int child_index = 0) {
		safe_assert(BETWEEN(0, child_index, children_.size()-1));
		// if no pred, then this means TRUE, so skip checking pred
		if(pred_ == NULL) {
			return true;
		}
		// check condition
		ThreadVarPtr var = this->var(child_index);
		safe_assert(var != NULL);
		Coroutine* current = safe_notnull(var.get())->thread();
		AuxState::Tid->set_thread(current);
		return (pred_->EvalPreState(current) == TPTRUE);
	}

	ThreadVarPtr create_thread_var(Coroutine* current = NULL) {
		std::stringstream s;
		if(static_info_->message() != "") {
			s << static_info_->message();
		} else if(current != NULL) {
			s << "THR-" << current->tid();
		} else {
			s << "ThreadVar";
		}

		ThreadVarPtr p(new ThreadVar(current, s.str()));
		return p;
	}

	virtual ThreadVarPtr var(int child_index = 0) = 0;

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/

class ExistsThreadNode : public SelectThreadNode {
public:
	ExistsThreadNode(StaticDSLInfo* static_info = NULL,
					 const TransitionPredicatePtr& pred = TransitionPredicatePtr(),
					 ExecutionTree* parent = NULL)
	: SelectThreadNode(static_info, pred, parent, 1), selected_tid_(-1), var_(create_thread_var()) {}

	~ExistsThreadNode() {}

	bool is_exists() { return true; }

	ChildLoc set_selected_thread(Coroutine* thread) {
		ChildLoc newnode = {this, 0};
		// set thread of the variable
		safe_assert(var_ != NULL);
		safe_assert(var_->thread() == NULL);
		var_->set_thread(thread);
		return newnode;
	}

	ChildLoc clear_selected_thread() {
		var_->set_thread(NULL);
		return ChildLoc::EMPTY();
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
		g->AddEdge(new DotEdge(node, cn, "?"));

		return node;
	}

	ThreadVarPtr var(int child_index = 0) {
		safe_assert(child_index == 0);
		safe_assert(var_ != NULL);
		return var_;
	}

	// override
	void OnConsumed(Coroutine* current, int child_index = 0) {
		ExecutionTree::OnConsumed(current, child_index);
		// update selected_tid
		selected_tid_ = current->tid();
	}

private:
	DECL_FIELD_SET(ThreadVarPtr, var)
	DECL_FIELD_REF(std::set<THREADID>, covered_tids)
	DECL_FIELD(THREADID, selected_tid)
};

/********************************************************************************/

class ForallThreadNode : public SelectThreadNode {
public:
	ForallThreadNode(StaticDSLInfo* static_info = NULL,
					 const TransitionPredicatePtr& pred = TransitionPredicatePtr(),
					 ExecutionTree* parent = NULL)
	: SelectThreadNode(static_info, pred, parent, 0) {}

	~ForallThreadNode() {}

	bool is_exists() { return false; }

	// override
	bool ComputeCoverage(bool call_parent = false);

	ChildLoc set_selected_thread(Coroutine* co) {
		int child_index = add_or_get_thread(co);
		safe_assert(BETWEEN(0, child_index, idxToThreadMap_.size()-1));
		ThreadVarPtr var = this->var(child_index);
		return {this, child_index};
	}

	ChildLoc set_selected_thread(int child_index) {
		safe_assert(BETWEEN(0, child_index, idxToThreadMap_.size()-1));
		ThreadVarPtr var = this->var(child_index);
		return {this, child_index};
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
			g->AddEdge(new DotEdge(node, cn, to_string(this->var(i).get()->thread()->tid())));
		}

		return node;
	}

	ThreadVarPtr var(int child_index = 0) {
		safe_assert(BETWEEN(0, child_index, idxToThreadMap_.size()-1));
		ThreadVarPtr var = idxToThreadMap_[child_index];
		safe_assert(var != NULL);
		safe_assert(!var->is_empty());
		return var;
	}

private:

	// returns the index of the new child
	int add_or_get_thread(Coroutine* co) {
		THREADID tid = safe_notnull(co)->tid();
		int child_index = child_index_by_tid(tid);
		if(child_index < 0) {
			const size_t sz = children_.size();
			safe_assert(idxToThreadMap_.size() == sz);
			tidToIdxMap_[tid] = sz;
			children_.push_back(NULL);
			ThreadVarPtr var = create_thread_var(co);
			idxToThreadMap_.push_back(var);
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
	DECL_FIELD_REF(std::vector<ThreadVarPtr>, idxToThreadMap)
};

/********************************************************************************/

//class SingleTransitionNode : public TransitionNode {
//public:
//	SingleTransitionNode(const TransitionPredicatePtr& assertion,
//						 const TransitionPredicatePtr& pred,
//						 const ThreadVarPtr& var = ThreadVarPtr(),
//						 const TransitionConstraintsPtr& constraints = TransitionConstraintsPtr(),
//						 const char* message = NULL,
//						 ExecutionTree* parent = NULL)
//	: TransitionNode(assertion, pred, var, constraints, message, parent, 1) {}
//
//	~SingleTransitionNode() {}
//
//	virtual void ToStream(FILE* file) {
//		fprintf(file, "TransitionNode.");
//		ExecutionTree::ToStream(file);
//	}
//
//	DotNode* UpdateDotGraph(DotGraph* g) {
//		DotNode* node = new DotNode("TransitionNode");
//		g->AddNode(node);
//		ExecutionTree* c = child();
//		DotNode* cn = NULL;
//		if(c != NULL) {
//			cn = c->UpdateDotGraph(g);
//		} else {
//			cn = new DotNode("NULL");
//		}
//		g->AddNode(cn);
//		g->AddEdge(new DotEdge(node, cn, "?"));
//		return node;
//	}
//};

/********************************************************************************/

class MultiTransitionNode : public TransitionNode {
public:
	MultiTransitionNode(StaticDSLInfo* static_info,
						 const TransitionPredicatePtr& assertion,
						 const TransitionPredicatePtr& pred,
						 const ThreadVarPtr& var = ThreadVarPtr(),
						 const TransitionConstraintsPtr& constraints = TransitionConstraintsPtr(),
						 ExecutionTree* parent = NULL, int num_children = 1)
	: TransitionNode(static_info, assertion, pred, var, constraints, parent, num_children) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "MultiTransitionNode.");
		ExecutionTree::ToStream(file);
	}
};

/********************************************************************************/

class TransferUntilNode : public MultiTransitionNode {
public:
	TransferUntilNode(StaticDSLInfo* static_info,
					 const TransitionPredicatePtr& assertion,
					 const TransitionPredicatePtr& pred,
					 const ThreadVarPtr& var = ThreadVarPtr(),
					 const TransitionConstraintsPtr& constraints = TransitionConstraintsPtr(),
					 ExecutionTree* parent = NULL)
	: MultiTransitionNode(static_info, assertion, pred, var, constraints, parent, 1) {}

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
		g->AddEdge(new DotEdge(node, cn, var_.get() == NULL ? "-" : var_.get()->ToString()));
		return node;
	}
};

/********************************************************************************/

const int ScheduleItem_ChildIndex = 1,
		  ScheduleItem_ThreadId = 2;

struct ScheduleItem {
	int kind_;
	int value_;

	ScheduleItem(int kind = 0, int value = -1) : kind_(kind), value_(value) {}
};

class PersistentSchedule : public std::vector<ScheduleItem>, public Serializable {
public:
	PersistentSchedule() : std::vector<ScheduleItem>() {}
	~PersistentSchedule() {}

	// override
	void Load(Serializer* serializer) {
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
	void Store(Serializer* serializer) {
		serializer->Store<int>(int(this->size()));
		for(iterator itr = begin(); itr < end(); ++itr) {
			serializer->Store<ScheduleItem>(*itr);
		}
		MYLOG(2) << "PersistentSchedule: Stored " << size() << " items";
	}

};

/********************************************************************************/

class ExecutionTreePath : public std::vector<ChildLoc> {
public:
	ExecutionTreePath() : std::vector<ChildLoc>() {}
	virtual ~ExecutionTreePath() {}

	PersistentSchedule* ComputeExecutionTreeStack(PersistentSchedule* schedule = NULL) {
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
};

/********************************************************************************/

class ExecutionTreeStack : public ExecutionTreePath {
public:
	ExecutionTreeStack() : ExecutionTreePath() {}
	~ExecutionTreeStack() {}

	void push(const ChildLoc& loc) {
		this->push_back(loc);
	}

	ChildLoc pop() {
		ChildLoc loc;
		safe_assert(loc == ChildLoc::EMPTY());
		if(this->size() > 0) {
			loc = this->back(); this->pop_back();
		}
		return loc;
	}

};

/********************************************************************************/

enum AcquireRefMode { EXIT_ON_EMPTY = 1, EXIT_ON_FULL = 2, EXIT_ON_LOCK = 3};

class ExecutionTreeManager {
public:
	ExecutionTreeManager();
	~ExecutionTreeManager();

	void Restart();
	void RestartChildIndexStack();

	inline ExecutionTree* ROOTNODE() { return &root_node_; }
	inline EndNode* ENDNODE() { return &end_node_; }
	inline LockNode* LOCKNODE() { return &lock_node_; }

	static inline bool IS_EMPTY(ExecutionTree* n) { return ((n) == NULL); }
	inline bool IS_FULL(ExecutionTree* n) { return !IS_EMPTY(n) && !IS_LOCKNODE(n) && !IS_ENDNODE(n); }
	inline bool IS_LOCKNODE(ExecutionTree* n) { return ((n) == (LOCKNODE())); }
	inline bool IS_ENDNODE(ExecutionTree* n) { bool b = (INSTANCEOF(n, EndNode*)); safe_assert(!b || (n == ENDNODE())); return b; }
	static inline bool IS_TRANSNODE(ExecutionTree* n) { return (INSTANCEOF(n, TransitionNode*)); }
	static inline bool IS_MULTITRANSNODE(ExecutionTree* n) { return (INSTANCEOF(n, MultiTransitionNode*)); }
	static inline bool IS_SELECTNODE(ExecutionTree* n) { return (INSTANCEOF(n, SelectionNode*)); }
	static inline bool IS_SELECTTHREADNODE(ExecutionTree* n) { return (INSTANCEOF(n, SelectThreadNode*)); }

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

	void AddToPath(ExecutionTree* node, int child_index);

	void AddToNodeStack(const ChildLoc& current);
	ChildLoc GetNextNodeInStack();
	ChildLoc GetLastNodeInStack();
	void TruncateNodeStack(int index);

	bool RestartForAlternatePath();
	int GetIndexInNodesStack(ChildLoc& loc);

	ExecutionTreePath* ComputePath(ChildLoc leaf_loc, ExecutionTreePath* path = NULL);
	bool DoBacktrack(BacktrackReason reason = SUCCESS) throw();

	bool EndWithSuccess(BacktrackReason* reason) throw();
	void EndWithException(Coroutine* current, std::exception* exception = NULL, const std::string& where = "<unknown>") throw();
	void EndWithBacktrack(Coroutine* current, BacktrackReason reason, const std::string& where) throw();

	bool CheckCompletePath(ExecutionTreePath* path);

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
	DECL_FIELD_REF(RootNode, root_node)
	DECL_FIELD_REF(LockNode, lock_node)
	DECL_FIELD_REF(StaticEndNode, end_node)

//	DECL_FIELD_GET_REF(ChildLoc, current_node) // no set method, use UpdateCurrentNode
	DECL_FIELD_REF(std::vector<ChildLoc>, current_nodes)
//	DECL_FIELD(unsigned, num_paths)

	ExecutionTreeRef atomic_ref_;
	Semaphore sem_ref_;

//	DECL_FIELD(Mutex, mutex)
//	DECL_FIELD(ConditionVar, cv)

	DECL_FIELD_REF(ExecutionTreeStack, replay_path)

	DECL_FIELD_REF(ExecutionTreeStack, node_stack)
	DECL_FIELD(int, stack_index)

	DISALLOW_COPY_AND_ASSIGN(ExecutionTreeManager)
};

/********************************************************************************/

} // namespace


#endif /* DSL_H_ */
