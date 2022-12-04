#pragma once

#include "chunk.hpp"
#include "object.hpp"
#include "str.hpp"


class ObjFunction : public Obj {
public:
    ObjFunction(Vm * vm, ObjString * funcName);
    ~ObjFunction();

    // implment Obj interface
    virtual ObjString * toString(Vm * vm) override;
    virtual void print() override;

public:
    int numInputs;  // number of expected parameters
    Chunk chunk;
    ObjString * name;  // function name
};

class ObjClosure : public Obj {
public:
    ObjClosure(Vm * vm, ObjFunction * function);
    ~ObjClosure();

    // implment Obj interface
    virtual ObjString * toString(Vm * vm) override;
    virtual void print() override;

public:
    ObjFunction * function;
};
