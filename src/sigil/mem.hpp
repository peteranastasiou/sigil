
#pragma once

#include "object.hpp"
#include "table.hpp"
#include "upvalue.hpp"

#include <stdint.h>
#include <vector>


// Predeclare Vm
class Vm;

/**
 * Memory Manager for all objects in a running instance of Sigil
 */
class Mem {
public:
    Mem();
    ~Mem();

    void init(Vm * vm);

    // Run garbage collector
    void collectGarbage();
    void addGrayObj(Obj * obj);

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

    // Persist an empty string as a special case for convenience/efficiency
    ObjString * EMPTY_STRING;
private:
    void freeObjects_();

    bool init_;
    Vm * vm_;
    Obj * objects_;     // linked list of objects
    ObjUpvalue * openUpvalues_;  // linked list of open upvalues
    StringSet internedStrings_;
    std::vector<Obj*> markedObjects_;  // gc marked objects
};
