#pragma once

// Predeclare references
class Vm;
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
        CLOSURE
    };

    Obj(Vm * vm, Type t);

    virtual ~Obj();

    virtual ObjString * toString(Vm * vm) = 0;
    virtual void print() = 0;

    Type type;
    Obj * next;  // linked list of all objects

private:
    Vm * vm_;
};
