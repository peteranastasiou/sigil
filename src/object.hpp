#pragma once

// Predeclare references
class Mem;
class ObjString;  // defined in str.hpp

/**
 * NOTE: if objects are all created via Vm, then we can do the register/deregister there, 
 * this may simplify ObjStrings
 */

struct Obj {
    enum Type {
        STRING,
        LIST,
        FUNCTION,
        CLOSURE,
        UPVALUE
    };

    Obj(Mem * mem, Type t);

    virtual ~Obj();

    virtual ObjString * toString() = 0;
    virtual void print(bool verbose) = 0;

    Type type;
    Obj * next;  // linked list of all objects

protected:
    Mem * const mem_;
};
