
#pragma once

#include <stdint.h>
#include "object.hpp"
#include "table.hpp"
#include "upvalue.hpp"

// Predeclare Vm
class Vm;

/**
 * Memory Manager for all objects in a running instance of Pond
 */
class Mem {
public:
    Mem(Vm * vm);
    ~Mem();

    // Run garbage collector
    void collectGarbage();

    // adding/removing objects, called from Obj(), ~Obj()
    void registerObj(Obj * obj);
    void deregisterObj(Obj * obj);

    // get open upvalues list
    ObjUpvalue * getRootOpenUpvalue(){ return openUpvalues_; }
    
    // Set the root of the open upvalue list
    void setRootOpenUpvalue(ObjUpvalue * upvalue){ openUpvalues_ = upvalue; }

    // Close all upvalues left on the end of the stack when the stack top moves to stackTop
    void closeUpvalues(Value * stackTop);

    // intern string helper
    StringSet * getInternedStrings(){ return &internedStrings_; }

private:
    void freeObjects_();

    Vm * vm_;
    Obj * objects_;     // linked list of objects
    ObjUpvalue * openUpvalues_;  // linked list of open upvalues
    StringSet internedStrings_;
};
