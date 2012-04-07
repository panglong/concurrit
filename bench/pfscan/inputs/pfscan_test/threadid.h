#ifndef _THREADID_H
#define _THREADID_H

#include "modeltypes.h"


class StringIdentifier {
    private:
        int id_array[MAXTHREADS];
        int threads_spawned;

        int get_id(int position);
        int spawn_thread();

    public:
        StringIdentifier(const StringIdentifier & parent);
        bool operator==(StringIdentifier rhs);
};





#endif
