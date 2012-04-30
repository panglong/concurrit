#include <stdio.h>

#include "multiset.h"

#define NUM_THREADS 3

#include "concurrit.h"


void* insertpair_routine1(void* arg) {
  multiset_t * multiset = (multiset_t*) arg;
  if(multiset_insert_pair(multiset, 1, 2))
	  printf("--------  1 and 2 INSERTED.\n");
  else
	  printf("--------  1 and 2 NOT INSERTED.\n");
  fflush(stdout);
  return NULL;
}

void* insertpair_routine2(void* arg) {
  multiset_t * multiset = (multiset_t*) arg;
  if(multiset_insert_pair(multiset, 3, 4))
  	  printf("--------  3 and 4 INSERTED.\n");
    else
  	  printf("--------  3 and 4 NOT INSERTED.\n");
  fflush(stdout);
  return NULL;
}

void* insertpair_routine3(void* arg) {
  multiset_t * multiset = (multiset_t*) arg;
  if(multiset_insert_pair(multiset, 5, 6))
  	  printf("--------  5 and 6 INSERTED.\n");
    else
  	  printf("--------  5 and 6 NOT INSERTED.\n");
  fflush(stdout);
  return NULL;
}

//============================================================//
//============================================================//

void* lookup_routine1(void* arg) {
  multiset_t * multiset = (multiset_t*) arg;
  int ret1 = multiset_lookup(multiset, 1);
  int ret2 = multiset_lookup(multiset, 2);
  concurritAssert(!ret1 || ret2);
  printf("--------  1 and 2 %s.\n", (ret1 ? "FOUND" : "NOT FOUND"));
  fflush(stdout);
  return NULL;
}

void* lookup_routine2(void* arg) {
  multiset_t * multiset = (multiset_t*) arg;
  int ret1 = multiset_lookup(multiset, 3);
  int ret2 = multiset_lookup(multiset, 4);
  concurritAssert(!ret1 || ret2);
  printf("--------  3 and 4 %s.\n", (ret1 ? "FOUND" : "NOT FOUND"));
  fflush(stdout);
  return NULL;
}

void* lookup_routine3(void* arg) {
  multiset_t * multiset = (multiset_t*) arg;
  int ret1 = multiset_lookup(multiset, 5);
  int ret2 = multiset_lookup(multiset, 6);
  concurritAssert(!ret1 || ret2);
  printf("--------  5 and 6 %s.\n", (ret1 ? "FOUND" : "NOT FOUND"));
  fflush(stdout);
  return NULL;
}

//============================================================//
//============================================================//

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MultisetScenario, "Multiset scenario")


	multiset_t multiset;

	//---------------------------------------------

	SETUP() {
		srand(time(NULL));
		multiset_init(&multiset);
	}

	//---------------------------------------------

	TEARDOWN() {
		concurritAssert(multiset.size <= 2*NUM_THREADS);
		concurritAssert(multiset.size%2 == 0);
	}

	TESTCASE() {

		ThreadVarPtr t_ip1 = CREATE_THREAD(insertpair_routine1, (void*)&multiset);
		ThreadVarPtr t_ip2 = CREATE_THREAD(insertpair_routine2, (void*)&multiset);
		ThreadVarPtr t_ip3 = CREATE_THREAD(insertpair_routine3, (void*)&multiset);

		ThreadVarPtr t_lu1 = CREATE_THREAD(lookup_routine1, (void*)&multiset);
		ThreadVarPtr t_lu2 = CREATE_THREAD(lookup_routine2, (void*)&multiset);
		ThreadVarPtr t_lu3 = CREATE_THREAD(lookup_routine3, (void*)&multiset);

		printf("=============================================\n"
			   "=============================================\n");

		//----------------------------------------------------------------------------------
		//----------------------------------------------------------------------------------

		FORALL(t1, BY(t_ip1) || BY(t_ip2) || BY(t_ip3));
		RUN_UNTIL(BY(t1), ENDS(), __);

		FORALL(t2, (BY(t_ip1) || BY(t_ip2) || BY(t_ip3)) && NOT(t1));
		RUN_UNTIL(BY(t2), ENDS(), __);

		FORALL(t3, (BY(t_ip1) || BY(t_ip2) || BY(t_ip3)) && NOT(t1) && NOT(t2));
		RUN_UNTIL(BY(t3), ENDS(), __);

		RUN_UNTIL(BY(t_lu2), ENDS(), __);
		RUN_UNTIL(BY(t_lu1), ENDS(), __);
		RUN_UNTIL(BY(t_lu3), ENDS(), __);

		WHILE_STAR
		{
			EXISTS(t);
			RUN_UNTIL(PTRUE && !FINAL(), FINAL(t))

		}
		RUN_UNTIL(PTRUE, FINAL())
	}

CONCURRIT_END_TEST(MultisetScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
