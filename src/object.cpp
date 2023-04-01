
#include "object.hpp"
#include "mem.hpp"

#include <stdio.h>


Obj::Obj(Mem * mem): mem_(mem) {
    isMarked = false;
    mem_->registerObj(this);
}

Obj::~Obj(){
    mem_->deregisterObj(this);
}

void Obj::gcMark() {
    if( !isMarked ){
        // printf("%p gc mark ", (void *)this);
        // print(false);
        // printf("\n");
    }
    isMarked = true;
}