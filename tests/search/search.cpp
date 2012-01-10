
#include "concurrit.h"
#include "math.h"

using namespace concurrit;


double A[] = {5.0, 3.0, 8.0};

double min_cost = 1e+10;
int min_item = -1;

// EXPENSIVE EXP
double compute_cost(int i){
  return exp(A[i]);
}

// LOWER BOUND FOR EXP
double lower_bound(int i){
  return 1.0 + A[i];
}

/************************************************************************/

  void* iteration(void* p) {
	int i = (*((int*)p));

//	YIELD("L1", READ(&min_cost));
	double m = YIELD_READ("L1", min_cost);

	// CHECK LOWER BOUND QUICKLY
    double b = lower_bound(i);
    if(b >= m) {
      return NULL;
    }
    // EXPENSIVE COMPUTATION
    double cost = compute_cost(i);
    // UPDATE MINIMUM
    if(cost < YIELD_READ("L2", min_cost)) {

    		YIELD_WRITE("L3", min_cost) = cost;
			min_item = i+1;
    }
    return NULL;
  }
/************************************************************************/

class SearchScenario : public Scenario {
public:

	SearchScenario(const char* name) : Scenario(name) {}

	virtual void SetUp() {
		min_cost = 1e+10;
		min_item = -1;
	}

	virtual void TearDown() {

	}

};

/************************************************************************/

class Scenario0 : public SearchScenario {
public:
	Scenario0() : SearchScenario("SearchTest0") {}

	void TestCase() {
		CheckForall();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		UntilEnd()->Transfer(iter1);
		UntilEnd()->Transfer(iter2);
		UntilEnd()->Transfer(iter3);

		Assert(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario1 : public SearchScenario {
public:
	Scenario1() : SearchScenario("SearchTest1") {}

	void TestCase() {
		CheckForall();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		DO {

			UntilEnd()->TransferStar();

		} UNTIL_ALL_END;

		Assert(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario2: public SearchScenario {
public:
	Scenario2() : SearchScenario("SearchTest2") {}

	void TestCase() {
		CheckForall();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		DO {

			UntilStar()->TransferStar();

		} UNTIL_ALL_END;

		Assert(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

// FAILS WITHOUT YIELDS
class Scenario3 : public SearchScenario {
public:
	Scenario3() : SearchScenario("SearchTest3") {}

	void TestCase() {
		CheckForall();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		for(int i = 0; i < 2; ++i) {
			UntilStar()->Transfer(iter1);
			UntilStar()->Transfer(iter2);
			UntilStar()->Transfer(iter3);
		}

		Assert(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario4 : public SearchScenario {
public:
	Scenario4() : SearchScenario("SearchTest4") {}

	void TestCase() {
		CheckForall();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		UntilEnd()->TransferStar();
		UntilEnd()->TransferStar();
		UntilEnd()->TransferStar();

		Assert(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario5 : public SearchScenario {
public:
	Scenario5() : SearchScenario("SearchTest5") {}

	void TestCase() {
		CheckForall();
		//CheckExists();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		UntilStar()->TransferStar();
		UntilStar()->TransferStar();
		UntilStar()->TransferStar();

		Assert(min_item == 2);
		//Assume(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario6 : public SearchScenario {
public:
	Scenario6() : SearchScenario("SearchTest6") {}

	void TestCase() {
		CheckForall();

		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		Until("L3")->Transfer(iter1);
		UntilStar()->TransferStar();

		UntilEnd()->TransferStar();
		UntilEnd()->TransferStar();

		Assume(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario7 : public SearchScenario {
public:
	Scenario7() : SearchScenario("SearchTest7") {}

	void TestCase() {
		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		{ WITHOUT(iter3);

			EXHAUSTIVE_SEARCH();
		}

		Assume(min_item == 2);

//		printf("Minimum cost: %f\n", min_cost);
//		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario8 : public SearchScenario {
public:
	Scenario8() : SearchScenario("SearchTest8") {}

	void TestCase() {
		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		CONTEXT_BOUNDED_EXHAUSTIVE_SEARCH(3);

		Assume(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

class Scenario9 : public SearchScenario {
public:
	Scenario9() : SearchScenario("SearchTest9") {}

	void TestCase() {
		int _0 = 0;
		int _1 = 1;
		int _2 = 2;

		coroutine_t iter1 = CreateThread("itr1", iteration, &_0);
		coroutine_t iter2 = CreateThread("itr2", iteration, &_1);
		coroutine_t iter3 = CreateThread("itr3", iteration, &_2);

		NDSEQ_SEARCH();

		Assume(min_item == 2);

		printf("Minimum cost: %f\n", min_cost);
		printf("Minimum index: %d\n", min_item);
	}
};

/************************************************************************/

int main(int argv, char** argc) {

	BeginCounit();

	Suite suite;

//	suite.AddScenario(new Scenario0());
//	suite.AddScenario(new Scenario1());
//	suite.AddScenario(new Scenario2());
//	suite.AddScenario(new Scenario3());
//	suite.AddScenario(new Scenario4());
//	suite.AddScenario(new Scenario5());
//	suite.AddScenario(new Scenario6());

//	suite.AddScenario(new Scenario7());
	suite.AddScenario(new Scenario8());
//	suite.AddScenario(new Scenario9());

	suite.RunAll();

	EndCounit();

	return 0;
}
