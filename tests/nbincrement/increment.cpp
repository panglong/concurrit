
#include "concurrit.h"
#include "math.h"

using namespace counit;

struct Counter {
	int x;
	int l;

	Counter() {
		x = 0;
		l = 0;
	}

	void increment() {

		while(true) {
			int t = YIELD_READ("L1", x);
			int k = t + 1;

			lock();
			if(YIELD_READ("L2", x) == t) {
				YIELD_WRITE("L3", x) = k;
				unlock();
				break;
			}
			unlock();
		}

	}

	void lock() {
		ASSUME(YIELD_READ("lock1", l) == 0);
		WRITE_YIELD("lock2", l) = 1;
//		while(true) {
//			if(YIELD_READ("lock1", l) == 0) {
//				WRITE_YIELD("lock2", l) = 1;
//				break;
//			} else {
//
//			}
//		}
	}

	void unlock() {
		WRITE_YIELD("unlock", l) = 0;
	}

	int get() {
		return x;
	}

private:
	bool CAS(int** t, int* v1, int* v2) {
//		YIELD_READ("CAS1", *t);
		bool ret = (*t == v1);
		if(ret) {
			*t = v2;
		}
//		YIELD_WRITE("CAS2", *t);
		return ret;
	}
};


