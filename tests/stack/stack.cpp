
#include "concurrit.h"
#include "math.h"

using namespace concurrit;

#define value_t int
#define EMPTY -1

struct Node {
	value_t data;
	Node *next;
	Node() {
		data = EMPTY;
		next = NULL;
	}
};

struct Stack {
	Node* top_;

	Stack() {
		top_ = NULL;
	}

	void push(value_t v) {
		assert(v >= 0);

		Node *t, *x;
		x = new Node();
		YIELD_WRITE("push1", x->data) = v;

		do {
			t = YIELD_READ("push2", top_);
			YIELD_WRITE("push3", x->next) = t;
		}
		while(!CAS(&top_,t,x));
	}

	value_t pop() {
		Node *t, *x;
		do {
			t = YIELD_READ("pop1", top_);
			if (t == NULL) {
				return EMPTY;
			}
			x = YIELD_READ("pop2", t->next);
		}
		while(!CAS(&top_,t,x));

		return YIELD_READ("pop3", t->data);
	}

	int size() {
		int s = 0;
		for(Node* node = top_; node != NULL; node = node->next) {
			s++;
		}
		return s;
	}

private:
	bool CAS(Node** t, Node* v1, Node* v2) {
		bool ret = (YIELD_READ("CAS1", *t) == v1);
		if(ret) {
			YIELD_WRITE("CAS2", *t) = v2;
		}
		return ret;
	}
};


