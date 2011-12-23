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

namespace counit {

/********************************************************************************/


class MemoryMap {
public:
	typedef std::map<ADDRINT, MemoryCellBase*> CellMap;
	MemoryMap() {}
	~MemoryMap() { delete_cells(); }

	void Update(MemoryMap* other);
	void Update(MemoryCellBase* cell);
	MemoryMap* Clone();

	std::string ToString();

private:
	void delete_cells();

	DECL_FIELD(CellMap, memToCell)
};


/********************************************************************************/
class EnvNode;
typedef boost::shared_ptr<EnvNode> EnvNodePtr;
typedef std::vector<EnvNodePtr> EnvNodePtrList;
typedef std::set<EnvNodePtr> EnvNodePtrSet;

// node in an env graph
class EnvNode {
public:
	EnvNode() {}
	~EnvNode() {}

	void OnAccess(Coroutine* current, MemoryCellBase* cell);
	void AddEdge(EnvNodePtr node);

	std::string ToString();

private:
	DECL_FIELD_REF(MemoryMap, globals)
	DECL_FIELD_REF(EnvNodePtrSet, edges)
};

/********************************************************************************/

class MemoryTrace {
public:
	MemoryTrace() {}
	~MemoryTrace() {
		delete_nodes();
	}

	void OnAccess(Coroutine* current, SharedAccess* access);
	void Restart();

	std::string ToString();

private:
	void delete_nodes();

	DECL_FIELD_REF(EnvNodePtrList, nodes)
};

/********************************************************************************/

class EnvGraph {
public:
	EnvGraph() {
		start_node_ = EnvNodePtr();
	}
	~EnvGraph() {}

	void AddNode(EnvNodePtr node);
	void Update(MemoryTrace* trace);

private:
	DECL_FIELD(EnvNodePtrSet, nodes)
	DECL_FIELD(EnvNodePtr, start_node)
};


/********************************************************************************/

class EnvSimulator {
public:
	EnvSimulator(EnvGraph* g) : env_graph_(g), current_(g->start_node()) {}
	~EnvSimulator() {}

	void Step() { unimplemented(); }

private:
	DECL_FIELD(EnvGraph*, env_graph)
	DECL_FIELD(EnvNodePtr, current)
};


/********************************************************************************/

class ThreadModularScenario : public Scenario {
public:
	typedef Scenario super;
	explicit ThreadModularScenario(const char* name) : Scenario(name) {}
	virtual ~ThreadModularScenario() {}

	virtual void OnAccess(Coroutine* current, SharedAccess* access);
	virtual void Restart();
	virtual void AfterRunOnce();

private:
	DECL_FIELD_REF(MemoryTrace, memory_trace)
	DECL_FIELD_REF(EnvGraph, env_graph)
};



} // end namespace

#endif /* MODULAR_H_ */
