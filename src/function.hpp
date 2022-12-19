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
    ObjFunction(Vm * vm, ObjString * funcName);
    ~ObjFunction();

    // implment Obj interface
    virtual ObjString * toString(Vm * vm) override;
    virtual void print(bool verbose) override;

public:
    int numInputs;  // number of expected parameters
    int numUpvalues;
    Chunk chunk;
    ObjString * name;  // function name
};

/**
 * Upvalues wrap ordinary Values when they are enclosed by a Closure
 */
class ObjUpvalue : public Obj {
public:
    ObjUpvalue(Vm * vm, Value * val);
    ~ObjUpvalue();

    inline void set(Value v) { *value = v; }
    inline Value get() { return *value; }

    // implment Obj interface
    virtual ObjString * toString(Vm * vm) override;
    virtual void print(bool verbose) override;

private:
    Value * value;
};

/**
 * Closures are made at runtime and wrap functions 
 * as well as enclosing Values, creating Upvalues
 */
class ObjClosure : public Obj {
public:
    ObjClosure(Vm * vm, ObjFunction * function);
    ~ObjClosure();

    // implment Obj interface
    virtual ObjString * toString(Vm * vm) override;
    virtual void print(bool verbose) override;

public:
    ObjFunction * function;
    std::vector<ObjUpvalue *> upvalues;
};
