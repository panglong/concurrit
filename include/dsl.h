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

#include <cstdatomic>

namespace concurrit {

class Coroutine;

enum TPVALUE { TPTRUE = 1, TPFALSE = 0, TPUNKNOWN = -1 };

class TransitionPredicate {
public:
	TransitionPredicate() {}
	virtual ~TransitionPredicate() {}
	virtual TPVALUE EvalPreState() = 0;
	virtual bool EvalPostState() = 0;
};

class TrueTransitionPredicate : public TransitionPredicate {
public:
	TrueTransitionPredicate() : TransitionPredicate() {}
	~TrueTransitionPredicate() {}
	TPVALUE EvalPreState() { return TPTRUE; }
	bool EvalPostState() { return TPTRUE; }
};

class FalseTransitionPredicate : public TransitionPredicate {
public:
	FalseTransitionPredicate() : TransitionPredicate() {}
	~FalseTransitionPredicate() {}
	TPVALUE EvalPreState() { return TPFALSE; }
	bool EvalPostState() { return TPFALSE; }
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ThreadVar {
public:
	ThreadVar(Coroutine* thread = NULL, std::string name = "<unknown>") : name_(name), thread_(thread) {}
	~ThreadVar(){}

private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(Coroutine*, thread)
};


/********************************************************************************/

class ExecutionTree;
typedef std::atomic_address ExecutionTreeRef;
typedef std::vector<ExecutionTree*> ExecutionTreeList;

#define for_each_child(child) \
	ExecutionTreeList::iterator __itr__ = children_.begin(); \
	ExecutionTree* child = (__itr__ != children_.end() ? *__itr__ : NULL); \
	for (; __itr__ != children_.end(); child = ((++__itr__) != children_.end() ? *__itr__ : NULL))

class ExecutionTree {
public:
	ExecutionTree(ExecutionTree* parent = NULL, int num_children = 0) : parent_(parent), covered_(false) {
		InitChildren(num_children);
	}

	virtual ~ExecutionTree();

	bool ContainsChild(ExecutionTree* node);

	void InitChildren(int n);

	ExecutionTree* child(int i = 0);

	virtual void ComputeCoverage(bool recurse);

private:
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD_REF(ExecutionTreeList, children)
	DECL_FIELD(bool, covered)

	friend class ExecutionTreeManager;
};

/********************************************************************************/

class ChoiceNode : public ExecutionTree {
public:
	ChoiceNode(ExecutionTree* parent = NULL) : ExecutionTree(parent, 2) {}
};

/********************************************************************************/

class SelectThreadNode : public ExecutionTree {
public:
	typedef std::map<THREADID, size_t> TidToIdxMap;

private:
	DECL_FIELD_REF(TidToIdxMap, tidToIdxMap)

};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode(TransitionPredicate* pred, ExecutionTree* parent = NULL) : ExecutionTree(parent, 1), pred_(pred) {}

private:
	DECL_FIELD(TransitionPredicate*, pred)
};

/********************************************************************************/

class TransferUntilNode : public ExecutionTree {
public:
	TransferUntilNode(TransitionPredicate* pred) : pred_(pred) {}

private:
	DECL_FIELD(TransitionPredicate*, pred)
};

/********************************************************************************/

class ExecutionTreeManager {
public:
	ExecutionTreeManager();

	void Restart();

inline bool REF_EMPTY(ExecutionTree* n) { return ((n) == NULL); }
inline bool REF_LOCKED(ExecutionTree* n) { return ((n) == (&lock_node_)); }

	// operations by script
	ExecutionTree* GetNextTransition();

	void SetNextTransition(ExecutionTree* node, ExecutionTree* next_node);

	// operations by clients

	// run by test threads to get the next transition node
	ExecutionTree* AcquireNextTransition();

	void ReleaseNextTransition(ExecutionTree* node, bool consumed);

	void ConsumeTransition(ExecutionTree* node);

	ExecutionTree* GetRef();

	ExecutionTree* ExchangeRef();

	void SetRef(ExecutionTree* node);

	ExecutionTree* GetLastInPath();

private:
	DECL_FIELD_REF(ExecutionTree, root_node)
	DECL_FIELD_REF(ExecutionTree, lock_node)
	ExecutionTreeRef atomic_ref_;
	DECL_FIELD(ExecutionTree*, current_node)

	DECL_FIELD_REF(std::vector<ExecutionTree*>, current_path)
	DECL_FIELD(unsigned, num_paths)

	DECL_FIELD(Mutex, mutex)
	DECL_FIELD(ConditionVar, cv)


	DISALLOW_COPY_AND_ASSIGN(ExecutionTreeManager)
};

/********************************************************************************/

enum TransitionConst {
	MEM_READ, MEM_WRITE,
	ENDING
};

// class storing information about a single transition
class TransitionInfo {
public:
	TransitionInfo(TransitionConst type, void* arg = NULL):
		type_(type), arg_(arg) {}
	~TransitionInfo(){}
private:
	DECL_FIELD(TransitionConst, type)
	DECL_FIELD(void*, arg)
};

} // namespace


#endif /* DSL_H_ */
