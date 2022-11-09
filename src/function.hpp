#pragma once

#include "chunk.hpp"
#include "object.hpp"
#include "str.hpp"


struct ObjFunction : public Obj {
    ObjFunction(Vm * vm, ObjString * funcName);
    ~ObjFunction();

    // implment Obj interface (trivial for strings)
    virtual ObjString * toString(Vm * vm) override;
    virtual void print() override;

    int arity;  // number of expected parameters
    Chunk chunk;
    ObjString * name;  // function name
};

