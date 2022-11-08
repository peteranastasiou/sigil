#pragma once

#include "chunk.hpp"
#include "object.hpp"
#include "str.hpp"


struct ObjFunction : public Obj {
    ObjFunction(Vm * vm);
    ~ObjFunction();

    // implment Obj interface (trivial for strings)
    virtual ObjString * toString() override { return name; }
    virtual void print() override { printf("<fn %s>", name->get()); }

    int arity;  // number of expected parameters
    Chunk chunk;
    ObjString * name;  // function name
};

