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

#include "counit.h"

namespace counit {

void ThreadModularScenario::OnAccess(Coroutine* current, SharedAccess* access) {
	super::OnAccess(current, access);

	// trace the access
	// if a read, update the current state
	// if a write, move to a new state
	env_trace_.OnAccess(current, access);
}

void ThreadModularScenario::Restart() {
	super::Restart();

	// restart memory trace
	env_trace_.Restart();
}

void ThreadModularScenario::AfterRunOnce() {
	super::AfterRunOnce();

	// print the memory trace:
	printf("Memory trace:\n%s\n", env_trace_.ToString().c_str());

	// add memory trace to env graph
	env_graph_.Update(&env_trace_);
}

/********************************************************************************/

void EnvTrace::OnAccess(Coroutine* current, SharedAccess* access) {
	EnvNodePtr node;
	if(access->is_read()) {
		// check if any state exists
		if(nodes_.empty()) {
			node = EnvNodePtr(new EnvNode());
			nodes_.push_back(node);
		} else {
			node = nodes_.back();
		}
	} else {
		node = EnvNodePtr(new EnvNode());
		if(!nodes_.empty()) {
			node->globals()->Update(nodes_.back()->globals());
		}
		nodes_.push_back(node);
	}
	// update the node
	node->Update(current, access->cell());
}

void EnvTrace::Restart(EnvGraph* g /*=NULL*/) {
	// delete all states
	delete_nodes();
	if(env_graph_ == NULL || g != NULL) {
		env_graph_ = g;
	}
	safe_assert(env_graph_ != NULL);
	current_ = env_graph_->start_node();
}

void EnvTrace::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}

std::string EnvTrace::ToString() {
	std::stringstream s;

	int i = 1;
	for(EnvNodePtrList::iterator itr = nodes_.begin(); itr < nodes_.end(); ++itr) {
		EnvNodePtr node = (*itr);
		s << i << " : " << node->ToString() << "\n";
		s << "********************\n";
		++i;
	}

	return s.str();
}

/********************************************************************************/

void EnvNode::Update(Coroutine* current, MemoryCellBase* cell) {
	// update globals
	globals_.Update(cell);
}

void EnvNode::AddEdge(EnvNodePtr node) {
	edges_.insert(node);
}

std::string EnvNode::ToString() {
	std::stringstream s;

	s << "MemoryMap: " << globals_.ToString();

	return s.str();
}

/********************************************************************************/

void MemoryMap::Update(MemoryMap* other) {
	for(CellMap::iterator itr = other->memToCell_.begin(); itr != other->memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		this->Update(cell);
	}
}

void MemoryMap::Update(MemoryCellBase* cell) {
	ADDRINT mem = cell->int_address();
	CellMap::iterator itr = memToCell_.find(mem);
	if(itr != memToCell_.end()) {
		itr->second->update_value(cell);
	} else {
		memToCell_[mem] = cell->Clone();
	}
}

std::string MemoryMap::ToString() {
	std::stringstream s;

	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		s << itr->second->ToString() << " ";
	}

	return s.str();
}

void MemoryMap::delete_cells() {
	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		delete cell;
		memToCell_.erase(itr);
	}
	safe_assert(memToCell_.empty());
}

MemoryMap* MemoryMap::Clone() {
	MemoryMap* other = new MemoryMap();
	for(CellMap::iterator itr = memToCell_.begin(); itr != memToCell_.end(); ++itr) {
		MemoryCellBase* cell = itr->second;
		other->Update(cell);
	}
	return other;
}

/********************************************************************************/

void EnvGraph::AddNode(EnvNodePtr node) {
	// we have at least start node
	safe_assert(start_node_ != NULL);
	nodes_.insert(node);
}

void EnvGraph::Update(EnvTrace* trace) {
	EnvNodePtr prev;
	// traverse the trace, and add new nodes and connections between nodes
	EnvNodePtrList* nodes = trace->nodes();
	for(EnvNodePtrList::iterator itr = nodes->begin(); itr < nodes->end(); ++itr) {
		EnvNodePtr node = (*itr);
		if(nodes_.find(node) == nodes_.end()) {
			// add the node
			AddNode(node);
		}
		if(prev != NULL) {
			// add edge from prev to node
			prev->AddEdge(node);
		}
		prev = node;
	}
}

void EnvGraph::delete_nodes() {
	// nodes are shared pointers, so do not delete them, just clear the list
	// shared_ptr takes care of the rest
	nodes_.clear();
}


} // end namespace
