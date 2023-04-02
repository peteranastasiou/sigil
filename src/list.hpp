#pragma once

#include "value.hpp"
#include "object.hpp"
#include <vector>

class ObjList : public Obj {
public:
    ObjList(Mem * mem);
    
    /**
     * Concatenation constructor:
     */
    ObjList(Mem * mem, ObjList * a, ObjList * b);

    ~ObjList();

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;
    virtual void gcMarkRefs() override;

    void append(Value v);
    bool get(int i, Value & v);
    bool set(int i, Value v);
    int len();

private:
    std::vector<Value> values_;
};

