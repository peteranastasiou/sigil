
#pragma once

#include <stdint.h>
#include "object.hpp"
#include "table.hpp"
#include "upvalue.hpp"

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

    // get open upvalue by local it references
    ObjUpvalue * getRootOpenUpvalue(){ return openUpvalues_; }
    
    // Set the root of the open upvalue list
    void setRootOpenUpvalue(ObjUpvalue * upvalue){ openUpvalues_ = upvalue; }

    // Close all upvalues above and including the value `last` on the variable stack
    void closeUpvalues(Value * last);

    // intern string helper
    StringSet * getInternedStrings(){ return &internedStrings_; }

private:
    void freeObjects_();

    Obj * objects_;     // linked list of objects
    ObjUpvalue * openUpvalues_;  // linked list of open upvalues
    StringSet internedStrings_;
};
