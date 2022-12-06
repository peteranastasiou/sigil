#pragma once

#include "value.hpp"
#include "object.hpp"
#include <vector>

class ObjList : public Obj {
public:
    ObjList(Vm * vm);
    
    /**
     * Concatenation constructor:
     */
    ObjList(Vm * vm, ObjList * a, ObjList * b);

    ~ObjList();

    // implment Obj interface
    virtual ObjString * toString(Vm * vm) override;
    virtual void print() override;

    void append(Value v);
    bool get(int i, Value & v);
    bool set(int i, Value v);
    int len();

private:
    std::vector<Value> values_;
};

