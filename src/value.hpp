#pragma once

#include "object.hpp"

// Predeclare object types
class ObjString;
class ObjList;
class ObjFunction;
class ObjClosure;

struct Value {
    /**
     * internal types
     */
    enum Type {
        // Primitive types:
        NIL = 0,  // Must be 0 so we can clear memory to NIL
        BOOL,
        NUMBER,  // TODO rename to FLOAT, add INT type
        TYPEID,
        // Garbage-Collected Object Types:
        STRING,
        LIST,
        FUNCTION,
        CLOSURE
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
    static inline Value list(Obj * o) { return (Value){LIST, {.obj = o}}; }
    static inline Value function(Obj * o) { return (Value){FUNCTION, {.obj = o}}; }
    static inline Value closure(Obj * o) { return (Value){CLOSURE, {.obj = o}}; }

    // Type to string
    static char const * typeToString(Type t);

    // Helpers for value types
    inline bool isNil() const { return type == NIL; }
    inline bool isBoolean() const { return type == BOOL; }
    inline bool isNumber() const { return type == NUMBER; }
    inline bool isTypeId() const { return type == TYPEID; }
    inline bool isString() const { return type == STRING; }
    inline bool isList() const { return type == LIST; }
    inline bool isFunction() const { return type == FUNCTION; }
    inline bool isClosure() const { return type == CLOSURE; }

    // As object helpers:
    ObjString * asObjString() const;
    ObjList * asObjList() const;
    ObjFunction * asObjFunction() const;
    ObjClosure * asObjClosure() const;

    // value methods
    bool equals(Value other) const;
    ObjString * toString(Vm * vm);
    void print() const;
};
