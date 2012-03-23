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

#ifndef MODULAR_H_
#define MODULAR_H_

#include "common.h"
#include "scenario.h"

namespace concurrit {

/********************************************************************************/

class MemoryMap {
public:
	typedef std::map<ADDRINT, MemoryCellBase*> CellMap;
	MemoryMap() {}
	~MemoryMap() { delete_cells(); }

	void Update(MemoryMap* other);
	void Update(MemoryCellBase* cell);
	MemoryMap* Clone();

	void UpdateProgramState();

	bool IsEquiv(const MemoryMap& other);
	bool operator ==(const MemoryMap& other) {
		return IsEquiv(other);
	}
	bool operator !=(const MemoryMap& other) {
		return !(this->operator ==(other));
	}

	std::string ToString();

private:
	void delete_cells();

	DECL_FIELD(CellMap, memToCell)
};


/********************************************************************************/
class EnvGraph;
class EnvNode;
typedef boost::shared_ptr<EnvNode> EnvNodePtr;
typedef std::vector<EnvNodePtr> EnvNodePtrList;
typedef std::set<EnvNodePtr> EnvNodePtrSet;

// a node in the env graph
class EnvNode {
public:
	EnvNode(EnvGraph* owner = NULL) : env_graph_(owner), id_(-1) {}
	~EnvNode() {}

	void Update(Coroutine* current, MemoryCellBase* cell);
	void AddEdge(EnvNodePtr node);
	EnvNodePtr Clone(bool clone_ginfo = false);

	bool IsEquiv(const EnvNode& node);
	bool operator ==(const EnvNode& node) {
		return IsEquiv(node);
	}
	bool operator !=(const EnvNode& node) {
		return !(this->operator ==(node));
	}

	std::string ToString(bool print_memory = true);

	inline bool HasOutEdges() {
		return !out_edges_.empty();
	}

	bool CheckAndUpdateProgramState();

private:
	DECL_FIELD_REF(MemoryMap, globals)
	DECL_FIELD_REF(EnvNodePtrSet, out_edges)
	// the graph this belongs to (can be null if the node is not owned yet)
	DECL_FIELD(EnvGraph*, env_graph)
	DECL_FIELD(int, id);
};

/********************************************************************************/
class EnvTrace;

class EnvGraph {
public:
	EnvGraph() {
		next_node_id_ = 1;
		start_node_ = EnvNodePtr(new EnvNode());
		AddNode(start_node_);
	}
	~EnvGraph() {
		delete_nodes();
	}

	EnvNodePtr MakeNewNode(EnvNodePtr existing = EnvNodePtr());
	void AddNode(EnvNodePtr node);
	bool ContainsNode(EnvNodePtr node);
	void Update(EnvTrace* trace);
	EnvNodePtr SearchForEquivNode(EnvNodePtr node);

	std::string ToString();

	size_t size() { return nodes_.size(); }

private:
	void delete_nodes();

	DECL_FIELD_REF(EnvNodePtrSet, nodes)
	DECL_FIELD(EnvNodePtr, start_node)
	DECL_FIELD(int, next_node_id)
};

/********************************************************************************/
class EnvChoicePoint;

// keeps track of the current trace of the execution
// this includes tracking where the environment is in the envgraph,
// and the linear path of envnodes visited during the execution
class EnvTrace {
public:
	// g may be NULL, in this case Restart is called later with a valid graph pointer
	EnvTrace(EnvGraph* g = NULL) {
		if(g != NULL) { // initialize only when g != NULL (not to fail an assertion in Restart)
			Restart(g);
		}
	}
	~EnvTrace() {
		delete_nodes();
	}

	// returns true, if no further step is possible
	EnvNodePtr Step(EnvChoicePoint* point);

	void SetNext(EnvNodePtr next_node);

	void OnAccess(Coroutine* current, SharedAccess* access);
	// if env_graph is not set or g is not NULL, we reset env_graph to g
	void Restart(EnvGraph* g = NULL);

	std::string ToString();

private:
	EnvNodePtr ChooseNext(EnvChoicePoint* point);
	void delete_nodes();

	DECL_FIELD_REF(EnvNodePtrList, nodes) // sequence of nodes visited
	DECL_FIELD(EnvGraph*, env_graph)
	DECL_FIELD(EnvNodePtr, current_node) // current node in the existing env graph

	friend class EnvChoicePoint;
};


/********************************************************************************/

class MemberChoicePoint : public ChoicePoint {
public:
	MemberChoicePoint(CoroutinePtrSet members,
					  CoroutinePtrSet::iterator itr,
					  Coroutine* source = Coroutine::Current())
	: ChoicePoint(source), members_(members), itr_(itr) {}
	MemberChoicePoint(CoroutinePtrSet members,
					  Coroutine* source = Coroutine::Current())
	: ChoicePoint(source), members_(members) { itr_ = members_.begin(); }

	~MemberChoicePoint() {}

	// override
	bool ChooseNext() {
		if(itr_ == members_.end()) {
			return false;
		}
		++itr_;
		return itr_ != members_.end();
	}

	Coroutine* GetNext() {
		return (itr_ != members_.end() ? (*itr_) : NULL);
	}

	virtual SchedulePoint* Clone() { return new MemberChoicePoint(members_, itr_, source_); }
	virtual void Load(Serializer* serializer) { unimplemented(); }
	virtual void Store(Serializer* serializer) { unimplemented(); }

private:
	DECL_FIELD(CoroutinePtrSet, members)
	DECL_FIELD(CoroutinePtrSet::iterator, itr)
};

/********************************************************************************/
typedef IteratorState<EnvNodePtr> EnvNodePtrIterator;
typedef std::vector<EnvNodePtrIterator> EnvNodePtrIteratorList;

class EnvChoicePoint : public ChoicePoint {
public:
	EnvChoicePoint(EnvTrace* env_trace,
				   EnvNodePtr root_node,
				   Coroutine* source = Coroutine::Current())
	: ChoicePoint(source),
	  env_trace_(env_trace), root_node_(root_node), source_(source) {
		path_.push_back(EnvNodePtrIterator(root_node_->out_edges()));
	}

	~EnvChoicePoint() {}

	// override
	bool ChooseNext() {
		env_trace_->Step(this);
		return (GetNext() != NULL);
	}

	inline EnvNodePtr GetNext() {
		if(path_.empty()) {
			return EnvNodePtr(); // NULL
		}
		EnvNodePtrIterator last = path_.back();
		if(last.has_current()) {
			return last.current();
		}
		return EnvNodePtr(); // NULL
	}

	virtual SchedulePoint* Clone() { return new EnvChoicePoint(env_trace_, root_node_, source_); } // TODO(elmas):...
	virtual void Load(Serializer* serializer) { unimplemented(); }
	virtual void Store(Serializer* serializer) { unimplemented(); }

private:
	DECL_FIELD(EnvTrace*, env_trace)
	DECL_FIELD(EnvNodePtr, root_node)
	DECL_FIELD_REF(EnvNodePtrIteratorList, path)
	DECL_FIELD(Coroutine*, source)

	DECL_FIELD_REF(EnvNodePtrSet, black_nodes)
	DECL_FIELD_REF(EnvNodePtrSet, gray_nodes)
};

/********************************************************************************/

class ThreadModularScenario : public Scenario {
public:
	typedef Scenario super;
	explicit ThreadModularScenario(const char* name) : Scenario(name) {
		// env_trace_ is created with NULL graph, so we initialize its graph pointer here.
		env_trace_.Restart(&env_graph_);
	}
	virtual ~ThreadModularScenario() {}

	void OnAccess(Coroutine* current, SharedAccess* access); // override
	void Start(); // override
	ConcurritException* RunOnce() throw(); // override
	void TestCase(); // override. implements the modular check, threads must be added in setup

private:
	DECL_FIELD_REF(EnvTrace, env_trace)
	DECL_FIELD_REF(EnvGraph, env_graph)
};



} // end namespace

#endif /* MODULAR_H_ */
