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

/********************************************************************************/

void DotGraph::AddNode(DotNode* node) {
	DotNodeMap::iterator itr = nodes_.find(node->id_);
	if(itr == nodes_.end()) {
		if(node->id_ < 0) {
			node->id_ = nodes_.size();
		}
		nodes_[node->id_] = node;
	} else {
		safe_assert(itr->second == node);
	}
}

/********************************************************************************/

void DotGraph::AddEdge(DotEdge* edge) {
	edges_.push_back(edge);
}

/********************************************************************************/

DotNode* DotGraph::GetNode(int id) {
	DotNodeMap::iterator itr = nodes_.find(id);
	if(itr != nodes_.end()) {
		return itr->second;
	}
	return NULL; // not found
}

/********************************************************************************/


DotGraph::~DotGraph() {
	for(DotNodeMap::iterator itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
		DotNode* node = itr->second;
		delete safe_notnull(node);
	}
	nodes_.clear();

	for(DotEdgeList::iterator itr = edges_.begin(); itr < edges_.end(); ++itr) {
		DotEdge* edge = *itr;
		delete safe_notnull(edge);
	}
	edges_.clear();
}


/********************************************************************************/

void DotGraph::Show(const char* filename /*= NULL*/) {
	if(filename == NULL) {
		filename = "/tmp/graph.dot";
	}

	WriteToFile(filename);

	execl("dot", filename, NULL);
}

/********************************************************************************/

void DotGraph::WriteToFile(const char* filename) {
	FILE* file = my_fopen(filename, "w");

	ToStream(file);

	my_fclose(file);
}

/********************************************************************************/


void DotGraph::ToStream(FILE* file) {
	fprintf(file, "digraph %s {\n", name_.c_str());

	for(DotNodeMap::iterator itr = nodes_.begin(); itr != nodes_.end(); ++itr) {
		DotNode* node = itr->second;
		node->ToStream(file);
		fprintf(file, "\n");
	}

	for(DotEdgeList::iterator itr = edges_.begin(); itr < edges_.end(); ++itr) {
		DotEdge* edge = *itr;
		edge->ToStream(file);
		fprintf(file, "\n");
	}

	fprintf(file, "}\n");
}

/********************************************************************************/

void DotNode::ToStream(FILE* file) {
	fprintf(file, "N%d [label=\"%s\", shape=%s, color=%s];",
			id_, label_.c_str(), shape_, color_);
}


/********************************************************************************/

void DotEdge::ToStream(FILE* file) {
	fprintf(file, "N%d -> N%d [label=\"%s\", style=%s, color=%s];",
			source_->id(), target_->id(), label_.c_str(), style_, color_);
}


/********************************************************************************/

} // end namespace
