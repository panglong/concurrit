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
#include "coroutine.h"

#include <cstdatomic>

namespace concurrit {


enum TPVALUE { TPTRUE = 1, TPFALSE = 0, TPUNKNOWN = -1 };

class DSLTransitionPredicate {
public:
	virtual TPVALUE EvalPreState() = 0;
	virtual bool EvalPostState() = 0;
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
typedef std::atomic<ExecutionTree*> ExecutionTreeRef;

class ExecutionTree {
public:
	ExecutionTree(){}
	virtual ~ExecutionTree(){}

private:
	DECL_FIELD(ExecutionTree*, parent)

	friend class ExecutionTreeManager;
};

/********************************************************************************/

class RootNode : public ExecutionTree {
public:
	RootNode() {
		next_node_.store(NULL);
	}

private:
	DECL_FIELD_GET(ExecutionTreeRef, next_node)

	friend class ExecutionTreeManager;
};

/********************************************************************************/

// indicates that a ref location is locked by a test thread
class LockNode : public ExecutionTree {
public:
	LockNode() {}
	~LockNode() {}
};

/********************************************************************************/

// indicates the end of the script
class LeafNode : public ExecutionTree {
public:
	LeafNode() {}
	~LeafNode() {}
};

/********************************************************************************/


class ChoiceNode : public ExecutionTree {
public:
	ChoiceNode() {
		true_node_.store(NULL);
		false_node_.store(NULL);
	}

private:
	DECL_FIELD_GET(ExecutionTreeRef, true_node)
	DECL_FIELD_GET(ExecutionTreeRef, false_node)
};


/********************************************************************************/

class SelectThreadNode : public ExecutionTree {
public:
	typedef std::map<THREADID, ExecutionTreeRef*> TidToNodeMap;

private:
	DECL_FIELD_REF(TidToNodeMap, tidToNodeMap)

};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode() {
		next_node_.store(NULL);
	}

private:
	DECL_FIELD(DSLTransitionPredicate*, expr)
	DECL_FIELD_GET(ExecutionTreeRef, next_node)
};

/********************************************************************************/

class TransferUntilNode : public ExecutionTree {
public:
	TransferUntilNode() {
		next_node_.store(NULL);
	}

private:
	DECL_FIELD(DSLTransitionPredicate*, expr)
	DECL_FIELD_GET(ExecutionTreeRef, next_node)
};

/********************************************************************************/

class ExecutionTreeManager {
public:
	ExecutionTreeManager() {
		current_ref_.store(&root_node_.next_node_);
	}

#define REF_EMPTY(n) 	((n) == NULL)
#define REF_LOCKED(n) 	((n) == (&lock_node_))
#define REF_FULL(n) 	((n) == (&root_node_))

	// operations by script




	// operations by clients

	// run by test threads to get the next transition node
	ExecutionTree* AcquireNextTransition() {
		ExecutionTree* node = NULL;
		while(true) {
			ExecutionTreeRef* current_ref = current_ref_.load();
			safe_assert(current_ref != NULL);
			while(true) {
				node = current_ref->exchange(&lock_node_);
				if(REF_EMPTY(node)) {
					// put node back
					current_ref->store(node);
					// TODO(elmas): wait/sleep
					continue;
				}
				else
				if(REF_LOCKED(node)) {
					// retry
					// TODO(elmas): wait/sleep
					continue;
				}
				else
				if(REF_FULL(node)) {
					// handle this
					node = contained_node_;
					contained_node_ = NULL;
					safe_assert(node != NULL);
					return node;
				}
				else {
					safe_assert(node != NULL);
					// put node back
					current_ref->store(node);
					// re-read current_ref
					// TODO(elmas): sleep
					break;
				}
			}
		}
		// unreachable
		safe_assert(false);
		return NULL;
	}

	void ReleaseConsumedTransition(ExecutionTree* node, ExecutionTreeRef* newref) {
		safe_assert(contained_node_ == NULL);

		// release
		ExecutionTreeRef* current_ref = current_ref_.load();
		ExecutionTree* ln = current_ref->exchange(node);
		safe_assert(REF_LOCKED(ln));

		// switch current reference
		current_ref_.store(newref);
	}


	void ReleaseUnconsumedTransition(ExecutionTree* node) {
		safe_assert(contained_node_ == NULL);

		// put back the node
		contained_node_ = node;

		// release
		ExecutionTreeRef* current_ref = current_ref_.load();
		ExecutionTree* ln = current_ref->exchange(node);
		safe_assert(REF_LOCKED(ln));
	}

private:
	DECL_FIELD_GET(RootNode, root_node)
	DECL_FIELD_GET(LockNode, lock_node)
	DECL_FIELD_GET(ExecutionTree*, contained_node)
	DECL_FIELD_GET(std::atomic<ExecutionTreeRef*>, current_ref)
};


} // namespace


#endif /* DSL_H_ */
