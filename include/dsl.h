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
	ExecutionTree(StaticDSLInfo* static_info = NULL, ExecutionTree* parent = NULL, int num_children = 0);

	virtual ~ExecutionTree();

	bool ContainsChild(ExecutionTree* node);

	void InitChildren(int n);

	ExecutionTree* child(int i = 0);
	int index_of(ExecutionTree* node);
	bool check_index(int i) { return BETWEEN(0, i, int(children_.size())-1); }

	void add_child(ExecutionTree* node);

	void set_child(ExecutionTree* node, int i = 0);
	ExecutionTree* get_child(int i);

	virtual bool child_covered(int i = 0);

	virtual bool ComputeCoverage(bool call_parent = false);

	virtual void ToStream(FILE* file) {
		fprintf(file, "Num children: %d, %s.", children_.size(), (covered_ ? "covered" : "not covered"));
	}

//	void PopulateLocations(int child_index, std::vector<ChildLoc>* current_nodes);

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
		if(exception_ == NULL && !exception_->contains(e)) {
			exception_ = new ConcurritException(e, owner, where, exception_);
		}
	}

	virtual void ToStream(FILE* file) {
		fprintf(file, "%s", ToString().c_str());
		ExecutionTree::ToStream(file);
	}

	std::string ToString() {
		const char* s = NULL;
		if(exception_ == NULL) {
			s = "SUCCESS";
		} else {
			BacktrackException* be = ASINSTANCEOF(exception_, BacktrackException*);
			if(be != NULL) {
				s = BacktrackException::ReasonToString(be->reason()).c_str();
			} else {
				AssertionViolationException* ae = ASINSTANCEOF(exception_, AssertionViolationException*);
				if(ae != NULL) {
					s = "ASSERT";
				} else {
					s = exception_->what();
				}
			}
		}
		safe_assert(s != NULL);
		return format_string("EndNode(%s)", s);
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
		safe_fail("StaticEndNode should not be deleted!!!");
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

class SelectionNode : public ExecutionTree {
public:
	SelectionNode(StaticDSLInfo* static_info = NULL, ExecutionTree* parent = NULL, int num_children = 0)
	: ExecutionTree(static_info, parent, num_children) {}
	virtual ~SelectionNode(){}
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

class ConditionalNode : public ExecutionTree {
public:
	ConditionalNode(StaticDSLInfo* info, bool value, ExecutionTree* parent = NULL)
	: ExecutionTree(info, parent, 1), value_(value) {}
	~ConditionalNode() {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "ConditionalNode: %s", bool_to_string(value_));
		ExecutionTree::ToStream(file);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode("ConditionalNode");
		g->AddNode(node);
		ExecutionTree* c = child(0);
		DotNode* cn = NULL;
		std::string edge_label;
		if(c != NULL) {
			cn = c->UpdateDotGraph(g);
			edge_label = std::string(bool_to_string(value_));
		} else {
			cn = new DotNode("NULL");
			edge_label = std::string("?");
		}
		g->AddNode(cn);
		g->AddEdge(new DotEdge(node, cn, edge_label));

		return node;
	}

private:
	DECL_FIELD(bool, value)
};

/********************************************************************************/

class StaticSelectThreadInfo : public StaticDSLInfo {
public:
	StaticSelectThreadInfo(ThreadVarPtrSet scope, SourceLocation* loc = NULL, const char* code = NULL) : StaticDSLInfo(loc, code), scope_(scope) {}
	~StaticSelectThreadInfo(){}

private:
	DECL_FIELD_REF(ThreadVarPtrSet, scope)
};

/********************************************************************************/

class SelectThreadNode : public SelectionNode {
public:
	SelectThreadNode(StaticDSLInfo* static_info = NULL,
					 ThreadVarPtrSet* scope = NULL,
					 const TransitionPredicatePtr& pred = TransitionPredicatePtr(),
					 ExecutionTree* parent = NULL, int num_children = 0)
	: SelectionNode(static_info, parent, num_children) , lvar_(create_thread_var()) {
		Init(scope, pred);
	}
	virtual ~SelectThreadNode() {}

	void Init(ThreadVarPtrSet* scope = NULL, const TransitionPredicatePtr& pred = TransitionPredicatePtr()) {

		scope_ = scope;
		pred_ = pred;

		// scope_size_ == 0 means scope is NULL, so use the total number of threads when needed
		scope_size_ = scope_ != NULL ? scope_->size() : 0;

		lvar_->clear_thread();
	}

	bool CanSelectThread(Coroutine* thread) {
		// if no pred, then this means TRUE, so skip checking pred
		if(pred_ == NULL || pred_ == TransitionPredicate::True()) {
			return true;
		}
		// check condition
		AuxState::Tid->set_thread(thread);
		return pred_->EvalState(thread);
	}

	ThreadVarPtr create_thread_var(Coroutine* current = NULL) {
		ThreadVarPtr t(new ThreadVar());
		if(current != NULL) {
			t->set_name("THR-" + current->tid());
			t->set_thread(current);
		}
		return t;
	}

	// override
	void OnConsumed(Coroutine* current, int child_index = 0) {
		ExecutionTree::OnConsumed(current, child_index);
		// update selected_tid
		safe_assert(lvar_->thread() == current);
//		selected_tid_ = current->tid();
	}

	virtual int CheckAndSelectThread(Coroutine* current, int child_index_in_stack = -1) = 0;
	virtual ThreadVarPtr& var(int child_index) = 0;

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
	DECL_FIELD(ThreadVarPtrSet*, scope)
	DECL_FIELD(size_t, scope_size)
	DECL_FIELD(ThreadVarPtr, lvar)
};

/********************************************************************************/

class ExistsThreadNode : public SelectThreadNode {
public:
	ExistsThreadNode(StaticDSLInfo* static_info = NULL,
					 ThreadVarPtrSet* scope = NULL,
					 const TransitionPredicatePtr& pred = TransitionPredicatePtr(),
					 ExecutionTree* parent = NULL)
	: SelectThreadNode(static_info, scope, pred, parent, 1) /*selected_tid_(-1)*/ {}

	~ExistsThreadNode() {}

	void ToStream(FILE* file) {
		fprintf(file, "ExistsThreadNode.");
		SelectThreadNode::ToStream(file);
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

	//override
	int CheckAndSelectThread(Coroutine* thread, int child_index_in_stack = -1) {
		THREADID tid = thread->tid();

		// EXISTS does not consider child_index_in_stack!
		safe_assert(child_index_in_stack == -1 || child_index_in_stack == 0);

		if(scope_ != NULL && !scope_->empty()) {
			ThreadVarPtr t = scope_->FindByThread(thread);
			if(t == NULL) {
				return -1;
			}
			safe_assert(thread == t->thread());

			if(covered_vars_.find(t) != covered_vars_.end()) {
				return -1;
			}
		} else {
			if(covered_vars_.FindByThread(thread) != NULL) {
				return -1;
			}
		}

		// check condition
		if(CanSelectThread(thread)) {
			lvar_->set_thread(thread);
			return 0;
		}

		safe_assert(lvar_->is_empty());
		return -1;
	}

	void UpdateCoveredVars() {
		safe_assert(lvar_ != NULL && !lvar_->is_empty());

		Coroutine* thread = lvar_->thread();
		ThreadVarPtr t;
		if(scope_ != NULL && !scope_->empty()) {
			t = scope_->FindByThread(thread);
		} else {
			t = create_thread_var(thread);
		}
		safe_assert(t != NULL && thread == t->thread());
		covered_vars_.insert(t);
	}

	//override
	ThreadVarPtr& var(int child_index) {
		safe_assert(child_index == 0);
		return lvar_;
	}

private:
	DECL_FIELD_REF(ThreadVarPtrSet, covered_vars)
//	DECL_FIELD(THREADID, selected_tid)
};

/********************************************************************************/

class ForallThreadNode : public SelectThreadNode {
	typedef std::map<ThreadVar*, int> ThreadVarToIdxMap;
public:
	ForallThreadNode(StaticDSLInfo* static_info = NULL,
					 ThreadVarPtrSet* scope = NULL,
					 const TransitionPredicatePtr& pred = TransitionPredicatePtr(),
					 ExecutionTree* parent = NULL)
	: SelectThreadNode(static_info, scope, pred, parent, 0) {}

	~ForallThreadNode() {}

	// override
	void ToStream(FILE* file) {
		fprintf(file, "ForallThreadNode.");
		SelectThreadNode::ToStream(file);
	}

	// override
	bool ComputeCoverage(bool call_parent = false);

	// override
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
			g->AddEdge(new DotEdge(node, cn, to_string(this->var(i)->thread()->tid())));
		}

		return node;
	}

	//override
	int CheckAndSelectThread(Coroutine* thread, int child_index_in_stack = -1) {
		THREADID tid = thread->tid();

		safe_assert(lvar_->is_empty());

		ThreadVarPtr t;
		if(scope_ != NULL && !scope_->empty()) {
			t = scope_->FindByThread(thread);
			if(t == NULL) {
				return -1;
			}
			safe_assert(thread == t->thread());
		} else {
			t = create_thread_var(thread);
		}

		// check condition
		if(!CanSelectThread(thread)) {
			return -1;
		}

		int child_index = add_or_get_child_index(t);
		safe_assert(check_index(child_index));
		safe_assert(child_index_in_stack == -1 || check_index(child_index_in_stack));

		if(child_index_in_stack >= 0 && child_index_in_stack != child_index) {
			return -1;
		}

		ExecutionTree* c = child(child_index);
		if(c != NULL && c->covered()) {
			return -1;
		}

		lvar_->set_thread(thread);
		return child_index;
	}

	// override
	ThreadVarPtr& var(int child_index) {
		safe_assert(BETWEEN(0, child_index, idxToThreadMap_.size()-1));
		ThreadVarPtr& var = idxToThreadMap_[child_index];
		safe_assert(var != NULL);
		safe_assert(!var->is_empty());
		return var;
	}

protected:

	// returns the index of the new child
	int add_or_get_child_index(const ThreadVarPtr& t) {
		ThreadVarToIdxMap::iterator itr = tvarToIdxMap_.find(t.get());
		if(itr == tvarToIdxMap_.end()) {
			const size_t sz = children_.size();
			safe_assert(idxToThreadMap_.size() == sz);
			tvarToIdxMap_[t.get()] = sz;
			children_.push_back(NULL);
			idxToThreadMap_.push_back(t);
			return sz;
		} else {
			int child_index = itr->second;
			safe_assert(BETWEEN(0, child_index, idxToThreadMap_.size()-1));
			return child_index;
		}
	}

private:
	DECL_FIELD_REF(ThreadVarToIdxMap, tvarToIdxMap)
	DECL_FIELD_REF(std::vector<ThreadVarPtr>, idxToThreadMap)
};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode(StaticDSLInfo* static_info,
				   const TransitionPredicatePtr& pred,
				   const ThreadVarPtr& var = ThreadVarPtr(),
				   ExecutionTree* parent = NULL, int num_children = 0)
	: ExecutionTree(static_info, parent, num_children) {
		Init(pred, var);
	}
	virtual ~TransitionNode(){}

	virtual void OnTaken(Coroutine* current, int child_index = 0);
	virtual void OnConsumed(Coroutine* current, int child_index = 0);

	void Init(const TransitionPredicatePtr& pred,
			   const ThreadVarPtr& var = ThreadVarPtr()) {
		pred_ = pred;
		var_ = var;
	}

	virtual const char* Kind() = 0;

	void ToStream(FILE* file) {
		fprintf(file, Kind());
		ExecutionTree::ToStream(file);
	}

	DotNode* UpdateDotGraph(DotGraph* g) {
		DotNode* node = new DotNode(Kind());
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

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

//class MultiTransitionNode : public TransitionNode {
//public:
//	MultiTransitionNode(StaticDSLInfo* static_info,
//						 const TransitionPredicatePtr& pred,
//						 const ThreadVarPtr& var = ThreadVarPtr(),
//						 ExecutionTree* parent = NULL, int num_children = 1)
//	: TransitionNode(static_info, pred, var, parent, num_children) {}
//
//	virtual void ToStream(FILE* file) {
//		fprintf(file, "MultiTransitionNode.");
//		ExecutionTree::ToStream(file);
//	}
//};


/********************************************************************************/

//class TransferNextNode : public TransitionNode {
//public:
//	TransferNextNode(StaticDSLInfo* static_info,
//					 const ThreadVarPtr& var = ThreadVarPtr(),
//					 ExecutionTree* parent = NULL)
//	: TransitionNode(static_info, TransitionPredicatePtr(), var, parent, 1) {}
//
//	~TransferNextNode() {}
//
//	const char* Kind() { return "TransferNextNode"; }
//};

/********************************************************************************/

class RunThroughNode : public TransitionNode {
public:
	RunThroughNode(StaticDSLInfo* static_info,
					 const TransitionPredicatePtr& pred,
					 const ThreadVarPtr& var = ThreadVarPtr(),
					 ExecutionTree* parent = NULL)
	: TransitionNode(static_info, pred, var, parent, 1) {}

	~RunThroughNode() {}

	const char* Kind() { return "RunThroughNode"; }
};

/********************************************************************************/

class RunUntilNode : public TransitionNode {
public:
	RunUntilNode(StaticDSLInfo* static_info,
					 const TransitionPredicatePtr& pred,
					 const ThreadVarPtr& var = ThreadVarPtr(),
					 ExecutionTree* parent = NULL)
	: TransitionNode(static_info, pred, var, parent, 1) {}

	~RunUntilNode() {}

	const char* Kind() { return "RunUntilNode"; }
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
	void Load(Serializer* serializer);

	//override
	void Store(Serializer* serializer);

};

/********************************************************************************/

class ExecutionTreePath : public std::vector<ChildLoc> {
public:
	ExecutionTreePath() : std::vector<ChildLoc>() {}
	virtual ~ExecutionTreePath() {}

	PersistentSchedule* ComputeExecutionTreeStack(PersistentSchedule* schedule = NULL);
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

	void AddToPath(ExecutionTree* node, int child_index);

	void AddToNodeStack(const ChildLoc& current);
	ChildLoc GetNextNodeInStack();
	ChildLoc GetLastNodeInStack();
	void TruncateNodeStack(int index);

//	bool RestartForAlternatePath();
	int GetIndexInNodesStack(ChildLoc& loc);

	ExecutionTreePath* ComputePath(ChildLoc leaf_loc, ExecutionTreePath* path = NULL);
	bool DoBacktrack(BacktrackReason reason = SUCCESS) throw();

	bool EndWithSuccess(BacktrackReason* reason) throw();
	void EndWithException(Coroutine* current, std::exception* exception = NULL, const std::string& where = "<unknown>") throw();
	void EndWithBacktrack(Coroutine* current, BacktrackReason reason, const std::string& where) throw();

	bool CheckCompletePath(ExecutionTreePath* path);

	void SaveDotGraph(const char* filename);

private:
	inline ExecutionTree* GetRef(std::memory_order mo = memory_order_seq_cst) {
		return static_cast<ExecutionTree*>(atomic_ref_.load(mo));
	}

	inline ExecutionTree* ExchangeRef(ExecutionTree* node, std::memory_order mo = memory_order_seq_cst) {
		return static_cast<ExecutionTree*>(atomic_ref_.exchange(node, mo));
	}

	// normally end node cannot be overwritten, unless overwrite_end flag is set
	inline void SetRef(ExecutionTree* node, std::memory_order mo = memory_order_seq_cst) {
		atomic_ref_.store(node, mo);
	}

private:
	DECL_FIELD_REF(RootNode, root_node)
	DECL_FIELD_REF(LockNode, lock_node)
	DECL_FIELD_REF(StaticEndNode, end_node)

//	DECL_FIELD_GET_REF(ChildLoc, current_node) // no set method, use UpdateCurrentNode
//	DECL_FIELD_REF(std::vector<ChildLoc>, current_nodes)
//	DECL_FIELD(unsigned, num_paths)

	ExecutionTreeRef atomic_ref_;
	Semaphore sem_ref_;

//	DECL_FIELD(Mutex, mutex)
//	DECL_FIELD(ConditionVar, cv)

//	DECL_FIELD_REF(ExecutionTreeStack, replay_path)

	DECL_FIELD_REF(ExecutionTreeStack, node_stack)
	DECL_FIELD(int, stack_index)

	DISALLOW_COPY_AND_ASSIGN(ExecutionTreeManager)
};

/********************************************************************************/

} // namespace


#endif /* DSL_H_ */
