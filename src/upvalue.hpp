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

    /**
     * Set the upvalue's value 
     * NOTE: the value is changed in the place it currently lives:
     *  either in the stack (if open) or in this object (once closed)
     */
    inline void set(Value v) { *value_ = v; }

    // Get the upvalue's value
    inline Value get() { return *value_; }

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;

private:
    // Private constructor: must construct with helper!
    ObjUpvalue(Mem * mem, Value * val);

    Value * value_;
    Value closedValue_;

    // linked list of upvalues so we don't double up on capturing locals:
    ObjUpvalue * nextUpvalue_;

    // TODO think about whether closeUpvalues belongs here, meaning Mem doesn't need to be a friend:
    // Or remove get/set on value
    friend class Mem;
};
