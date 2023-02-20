#pragma once

// Predeclare references
class Mem;
class ObjString;  // defined in str.hpp

/**
 * NOTE: if objects are all created via Vm, then we can do the register/deregister there, 
 * this may simplify ObjStrings
 */

struct Obj {
    Obj(Mem * mem);

    virtual ~Obj();

    virtual ObjString * toString() = 0;
    virtual void print(bool verbose) = 0;

    Obj * next;  // linked list of all objects

protected:
    Mem * const mem_;
};
