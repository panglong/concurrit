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

#ifndef DOT_H_
#define DOT_H_

#include "common.h"

namespace concurrit {

#define DotShape_Box 		"box"
#define DotShape_Circle 	"circle"

#define DotColor_Black		"black"
#define DotColor_Blue		"blue"

#define DotStyle_Solid		"solid"
#define DotStyle_Dashed		"dashed"
#define DotStyle_Dotted		"dotted"


class DotNode : public Writable {
public:
	DotNode(std::string label, int id = -1)
	: label_(label), id_(id), shape_(DotShape_Box), color_(DotColor_Blue) {}
	~DotNode() {}

	void ToStream(FILE* file);

private:
	DECL_FIELD(std::string, label)
	DECL_FIELD(int, id)
	DECL_FIELD(const char*, shape)
	DECL_FIELD(const char*, color)

	friend class DotGraph;
};

/********************************************************************************/

class DotEdge : public Writable {
public:
	DotEdge(DotNode* source, DotNode* target, std::string label = "")
	: label_(label), source_(source), target_(target), style_(DotStyle_Solid), color_(DotColor_Black) {}
	~DotEdge() {}

	void ToStream(FILE* file);

private:
	DECL_FIELD(std::string, label)
	DECL_FIELD(DotNode*, source)
	DECL_FIELD(DotNode*, target)
	DECL_FIELD(const char*, style)
	DECL_FIELD(const char*, color)

	friend class DotGraph;
};

/********************************************************************************/

typedef std::map<int, DotNode*> DotNodeMap;
typedef std::vector<DotEdge*> DotEdgeList;

class DotGraph : public Writable {
public:
	DotGraph(const std::string& name) : name_(name) {}
	~DotGraph();

	void AddNode(DotNode* node);
	void AddEdge(DotEdge* edge);
	DotNode* GetNode(int id);

	void ToStream(FILE* file);

	void WriteToFile(const char* filename);

	void Show(const char* filename = NULL);

private:
	DECL_FIELD(std::string, name)
	DECL_FIELD_REF(DotNodeMap, nodes);
	DECL_FIELD_REF(DotEdgeList, edges)
};

/********************************************************************************/

} // end namespace

#endif /* COROUTINEGROUP_H_ */
