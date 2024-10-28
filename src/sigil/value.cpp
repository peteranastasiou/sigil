
#include "value.hpp"
#include "str.hpp"
#include "function.hpp"
#include "list.hpp"

#include <stdio.h>


ObjString * Value::asObjString() const {
    return (ObjString *) as.obj;
}

ObjList * Value::asObjList() const {
    return (ObjList *) as.obj;
}

ObjFunction * Value::asObjFunction() const {
    return (ObjFunction *) as.obj;
}

ObjClosure * Value::asObjClosure() const {
    return (ObjClosure *) as.obj;
}

ObjUpvalue * Value::asObjUpvalue() const {
    return (ObjUpvalue *) as.obj;
}

char const* Value::typeToString(Type t) {
    switch( t ){
        case NIL:      return "nil";
        case BOOL:     return "bool";
        case NUMBER:   return "float";
        case TYPEID:   return "typeid";
        case FUNCTION: return "function";
        case CLOSURE:  return "closure";
        case UPVALUE:  return "upvalue";
        case LIST:     return "list";
        case STRING:   return "string";
        default:       return "???";   // Unreachable
    }
}

void Value::gcMark() {
    switch( type ){
        default:
        case NIL:
        case BOOL:
        case NUMBER:
        case TYPEID:
            // Primitive type - nothing to do
            return;

        case STRING:
        case LIST:
        case FUNCTION:
        case CLOSURE:
        case UPVALUE:
            // GC Object types:
            as.obj->gcMark();
            return;
    }
}

bool Value::equals(Value other) const {
    if( type != other.type ) return false;

    switch( type ){
        case NIL:     return true;
        case BOOL:    return as.boolean == other.as.boolean;
        case NUMBER:  return as.number == other.as.number;
        case TYPEID:  return as.typeId == other.as.typeId;
        case FUNCTION:  // function is only equal if its the exact same identity:
        case CLOSURE:   // TODO check if correct
        case UPVALUE:   // TODO check if correct
        case LIST:      // same for list, might be self referential so no safe way to deep inspect
        case STRING:    // all strings are interned --> therefore can compare pointers
            return as.obj == other.as.obj;
        default: return false;   // Unreachable
    }
}

ObjString * Value::toString(Mem * mem) {
    switch( type ){
        case NIL:     return ObjString::newString(mem, "nil");
        case BOOL:    return ObjString::newString(mem, as.boolean ? "true" : "false");
        case NUMBER:  return ObjString::newStringFmt(mem, "%g", as.number);
        case TYPEID:  return ObjString::newString(mem, typeToString(as.typeId));
        case FUNCTION:
        case CLOSURE:
        case UPVALUE:
        case LIST:
        case STRING:
            // Object types:
            return as.obj->toString();
        default:      return ObjString::newString(mem, "???");
    }
}

void Value::print(bool verbose) const {
    switch( type ){
        case NIL:     printf("nil"); return;
        case BOOL:    printf(as.boolean ? "true" : "false"); return;
        case NUMBER:  printf("%g", as.number); return;
        case TYPEID:  printf("%s", typeToString(as.typeId)); return;
        case FUNCTION:
        case CLOSURE:
        case UPVALUE:
        case LIST:
        case STRING:
            // Object types:
            as.obj->print(verbose); return;
        default:      printf("?%i", (int)type);
    }
}
