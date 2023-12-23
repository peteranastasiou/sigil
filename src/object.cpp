
#include "object.hpp"
#include "mem.hpp"

#include <stdio.h>


Obj::Obj(Mem * mem): mem_(mem) {

#ifdef DEBUG_GC
    printf("New obj at %p\n", this);
#endif

    isMarked = false;
    mem_->registerObj(this);
}

Obj::~Obj(){
}

void Obj::gcMark() {
    if( isMarked ){
        return;
    }
    isMarked = true;

    // TODO optimise by not adding objects to gray list which we know are leaves e.g. strings

    // Add to list of gray objects
    mem_->addGrayObj(this);
}