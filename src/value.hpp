#pragma once

#include "object.hpp"
#include "str.hpp"
#include <string>
#include <string.h>

struct Value {
    /**
     * internal types
     */
    enum Type {
        NIL,
        BOOL,
        NUMBER,  // TODO rename to FLOAT
        OBJECT,
        STRING,  // Note: string is implemented as an Object
        TYPEID
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
    static inline Value object(Obj * o) { return (Value){STRING, {.obj = o}}; }
    static inline Value typeId(Type t) {
        if( t == NIL ) return Value::nil();     // type(nil) == nil
        return (Value){TYPEID, {.typeId = t}};
    }

    // Helpers for value types
    inline bool isNil() const { return type == NIL; }
    inline bool isBoolean() const { return type == BOOL; }
    inline bool isNumber() const { return type == NUMBER; }
    inline bool isObject() const { return type == OBJECT || type == STRING; }
    inline bool isString() const { return type == STRING; }
    inline bool isTypeId() const { return type == TYPEID; }

    // Helpers for object types
    inline bool isObjType(Obj::Type t) const { return isObject() && as.obj->type == t; }
    inline ObjString * asObjString() const { return (ObjString*)as.obj; }
    inline char const * asCString() const { return asObjString()->get(); }

    // value methods
    bool equals(Value other) const;
    ObjString * toString(Vm * vm);
    void print() const;
};
