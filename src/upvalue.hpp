#pragma once

#include "value.hpp"
#include "object.hpp"

/**
 * Upvalues wrap ordinary Values when they are enclosed by a Closure
 */
class ObjUpvalue : public Obj {
public:
    ObjUpvalue(Mem * mem, Value * val);
    ~ObjUpvalue();

    inline void set(Value v) { *value_ = v; }
    inline Value get() { return *value_; }

    inline Value * getPtr() { return value_; }

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;

private:
    Value * value_;

    // linked list of upvalues so we don't double up on capturing locals:
    ObjUpvalue * next_;

    friend class Mem;
};
