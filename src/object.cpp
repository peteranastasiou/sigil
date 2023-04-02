
#include "object.hpp"
#include "mem.hpp"

#include <stdio.h>


Obj::Obj(Mem * mem): mem_(mem) {
    isMarked = false;
    mem_->registerObj(this);
}

Obj::~Obj(){
}

void Obj::gcMark() {
    if( isMarked ){
        return;
    }
    // printf("%p gc mark ", (void *)this);
    // print(false);
    // printf("\n");
    isMarked = true;

    // Add to list of gray objects
    mem_->addGrayObj(this);
}