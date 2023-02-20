#pragma once

#include "value.hpp"
#include "object.hpp"

/**
 * Upvalues wrap ordinary Values when they are enclosed by a Closure
 */
class ObjUpvalue : public Obj {
public:
    /**
     * Constructor helper - returns existing upvalue or makes a new one
     */
    static ObjUpvalue * newUpvalue(Mem * mem, Value * local);

    ~ObjUpvalue();

    inline void set(Value v) { *value_ = v; }
    inline Value get() { return *value_; }

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;

private:
    // Private constructor: must construct with helper!
    ObjUpvalue(Mem * mem, Value * val);

    Value * value_;

    // linked list of upvalues so we don't double up on capturing locals:
    ObjUpvalue * nextUpvalue_;
};
