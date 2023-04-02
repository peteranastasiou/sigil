#pragma once

#include "chunk.hpp"
#include "object.hpp"
#include "str.hpp"
#include <vector>


/**
 * Functions are made at compile time
 */
class ObjFunction : public Obj {
public:
    ObjFunction(Mem * mem, ObjString * funcName);
    ~ObjFunction();

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;
    virtual void gcMarkRefs() override;

public:
    int numInputs;  // number of expected parameters
    int numUpvalues;
    Chunk chunk;
    ObjString * name;  // function name
};


/**
 * Closures are made at runtime and wrap functions 
 * as well as enclosing Values, creating Upvalues
 */
class ObjClosure : public Obj {
public:
    ObjClosure(Mem * mem, ObjFunction * function);
    ~ObjClosure();

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;
    virtual void gcMarkRefs() override;

public:
    ObjFunction * function;
    std::vector<ObjUpvalue *> upvalues;
};
