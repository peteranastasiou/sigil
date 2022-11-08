#pragma once

#include "object.hpp"

// Predeclare object types
class ObjString;
struct ObjFunction;

struct Value {
    /**
     * internal types
     */
    enum Type {
        // Primitive types:
        NIL,
        BOOL,
        NUMBER,  // TODO rename to FLOAT, add INT type
        TYPEID,
        // Garbage-Collected Object Types:
        STRING,
        FUNCTION
    } type;

    union {
        bool boolean;
        double number;
        Obj * obj;
        Type typeId;
    } as;

    // Constructor-likes:
    static inline Value nil() { return (Value){NIL, {.number = 0}}; }
    static inline Value boolean(bool b) { return (Value){BOOL, {.boolean = b}}; }
    static inline Value number(double n) { return (Value){NUMBER, {.number = n}}; }
    static inline Value typeId(Type t) {
        if( t == NIL ) return Value::nil();     // We want type(nil) == nil
        return (Value){TYPEID, {.typeId = t}};
    }
    static inline Value string(Obj * o) { return (Value){STRING, {.obj = o}}; }
    static inline Value function(Obj * o) { return (Value){FUNCTION, {.obj = o}}; }

    // Helpers for value types
    inline bool isNil() const { return type == NIL; }
    inline bool isBoolean() const { return type == BOOL; }
    inline bool isNumber() const { return type == NUMBER; }
    inline bool isTypeId() const { return type == TYPEID; }
    inline bool isString() const { return type == STRING; }
    inline bool isFunction() const { return type == FUNCTION; }

    // As object helpers:
    ObjString * asObjString() const;
    ObjFunction * asObjFunction() const;

    // value methods
    bool equals(Value other) const;
    ObjString * toString(Vm * vm);
    void print() const;
};
