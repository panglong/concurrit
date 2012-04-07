#ifndef _TLSMODELDATA_H_
#define _TLSMODELDATA_H_

#include "ThreadInfo.h"
#include "modeltypes.h"

class TLSModelData : public ThreadInfo {
public:
    int myid;
    int parentid;    
    TLSModelData(int me, int parent) : ThreadInfo(0) {
        myid = me;
        parentid = parent;
    }
};


#endif

