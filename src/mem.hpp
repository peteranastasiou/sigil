
#pragma once

#include <stdint.h>
#include "object.hpp"
#include "table.hpp"

/**
 * Memory Manager for all objects in a running instance of Pond
 */
class Mem {
public:
    Mem();
    ~Mem();

    // adding/removing objects, called from Obj(), ~Obj()
    void registerObj(Obj * obj);
    void deregisterObj(Obj * obj);

    // intern string helper
    StringSet * getInternedStrings(){ return &internedStrings_; }

private:
    void freeObjects_();

    Obj * objects_;     // linked list of objects
    StringSet internedStrings_;
};
