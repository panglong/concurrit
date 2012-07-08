#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <libmemcached/memcached.h>

#include "interface.h"
#include "ipc.h"

const char* g_host = "127.0.0.1";
unsigned short g_port = 11211;
bool g_binary = false;

const char* g_key = "test";
size_t g_key_len = 4;

unsigned int g_loop = 10;
unsigned int g_threads = 10;

static memcached_st* create_st()
{
	memcached_st* st = memcached_create(NULL);
	if(!st) {
		perror("memcached_create failed");
		exit(1);
	}

	char* hostbuf = strdup(g_host);
	memcached_server_add(st, hostbuf, g_port);
	free(hostbuf);

	if(g_binary) {
		memcached_behavior_set(st, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
	}

	return st;
}

static void* worker(void* trash)
{
	int i;
	uint64_t value;

	memcached_st* st = create_st();

	for(i=0; i < g_loop; ++i) {
		memcached_increment(st, g_key, g_key_len, 1, &value);
	}

	memcached_free(st);
	return NULL;
}

int main(int argc, char* argv[])
{
	fprintf(stderr, "HARNESS: Openning pipe\n");

	concurrit::ConcurrentPipe* controlpipe = concurrit::ConcurrentPipe::OpenControlForSUT(NULL, false);

	fprintf(stderr, "HARNESS: Starting\n");

	//====================================================
	//====================================================

	for(;;) {

		{
			memcached_return ret;
			memcached_st* st = create_st();

			ret = memcached_set(st, g_key, g_key_len, "0",1, 0, 0);
			if(ret != MEMCACHED_SUCCESS) {
				fprintf(stderr, "set failed: %s\n", memcached_strerror(st, ret));
			}

			memcached_free(st);
		}

		//====================================================
		//====================================================

		fprintf(stderr, "HARNESS: Sending TestStart event\n");

		concurrit::EventBuffer e;
		e.type = concurrit::TestStart;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		controlpipe->Recv(NULL, &e);
		safe_assert(e.type == Continue);

		fprintf(stderr, "HARNESS: Sent TestStart event\n");

		//====================================================
		//====================================================

		{
			int i;
			pthread_t threads[g_threads];

			for(i=0; i < g_threads; ++i) {
				int err = pthread_create(&threads[i], NULL, worker, NULL);
				if(err != 0) {
					fprintf(stderr, "failed to create thread: %s\n", strerror(err));
					exit(1);
				}
			}

			for(i=0; i < g_threads; ++i) {
				void* ret;
				fprintf(stderr, "HARNESS: Joining thread %d\n", i);
				int err = pthread_join(threads[i], &ret);
				if(err != 0) {
					fprintf(stderr, "failed to join thread: %s\n", strerror(err));
				}
			}
		}

		fprintf(stderr, "HARNESS: Joined threads\n");

		{
			memcached_return ret;
			char* value;
			size_t vallen;
			uint32_t flags;
			memcached_st* st = create_st();

			value = memcached_get(st, g_key, g_key_len,
					&vallen, &flags, &ret);
			if(ret != MEMCACHED_SUCCESS) {
				fprintf(stderr, "get failed: %s\n", memcached_strerror(st, ret));
			}

					int expected_value = g_threads * g_loop;
					int retrieved_value = atoi(value);

					printf("expected: %d\n", expected_value);
			printf("result:   %d\n", retrieved_value);

			memcached_free(st);
					assert(expected_value == retrieved_value);
		}
	
		//====================================================
		//====================================================

		e.type = concurrit::TestEnd;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		controlpipe->Recv(NULL, &e);
		safe_assert(e.type == Continue);

		fprintf(stderr, "HARNESS: Sent TestEnd event\n");

	} // end for

	controlpipe->Close();

	delete(controlpipe);

	fprintf(stderr, "HARNESS: Exiting\n");

	return 0;
}

////
// result:
//
// $ memcached -U 0 -p 11211 -t 4 -d
//
// $ ./a.out 
// expected: 100000
// result:   99996
//
// $ ./a.out 
// expected: 100000
// result:   99998
//
// $ ./a.out 
// expected: 100000
// result:   99998
//
// - Mac OS X 10.6.2 (Snow Leopard)
// - memcached-1.4.4   (Mach-O 64-bit executable x86_64)
// - libevent-1.4.13
// - libmemcached-0.37
//
