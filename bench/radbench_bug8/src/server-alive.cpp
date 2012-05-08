#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <libmemcached/memcached.h>

const char* g_host = "127.0.0.1";
unsigned short g_port = 11211;
bool g_binary = false;

const char* g_key = "blah";
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

int main(int argc, char* argv[])
{
    {
        memcached_return ret;
        memcached_st* st = create_st();

        ret = memcached_set(st, g_key, g_key_len, "0", 1, 0, 0);
        memcached_free(st);
        if(ret != MEMCACHED_SUCCESS) {
            return 1;
        } else {
            return 0;
        }
    }

}

