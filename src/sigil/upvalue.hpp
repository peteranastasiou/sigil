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

    // Get a reference to the upvalue's value
    inline Value * ref() { return value_; }

    /** close the upvalue, so it uses an internal value
     * rather than referencing one on the variable stack
     */
    void close();

    // Get the next upvalue in the linked list of upvalues
    inline ObjUpvalue * getNextUpvalue() { return nextUpvalue_; }

    // implment Obj interface
    virtual ObjString * toString() override;
    virtual void print(bool verbose) override;
    virtual void gcMarkRefs() override;

private:
    // Private constructor: must construct with helper!
    ObjUpvalue(Mem * mem, Value * val);

    Value * value_;
    Value closedValue_;

    // linked list of upvalues so we don't double up on capturing locals:
    ObjUpvalue * nextUpvalue_;
};
