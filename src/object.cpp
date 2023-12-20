
#include "object.hpp"
#include "mem.hpp"

#include <stdio.h>


Obj::Obj(Mem * mem): mem_(mem) {
    printf("New obj at %p\n", this);
    isMarked = false;
    mem_->registerObj(this);
}

Obj::~Obj(){
}

void Obj::gcMark() {
    if( isMarked ){
        return;
    }
    printf("gc mark %p\n", (void *)this);
    isMarked = true;

    // TODO optimise by not adding objects to gray list which we know are leaves e.g. strings

    // Add to list of gray objects
    mem_->addGrayObj(this);
}