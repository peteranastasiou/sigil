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
    int arity;  // number of expected parameters
    Chunk chunk;
    ObjString * name;  // function name
};

