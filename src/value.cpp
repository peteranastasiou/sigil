
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

static char const* typeIdToString(Value::Type t) {
    switch( t ){
        case Value::NIL:      return "nil";
        case Value::BOOL:     return "bool";
        case Value::NUMBER:   return "float";
        case Value::TYPEID:   return "typeid";
        case Value::FUNCTION: return "function";
        case Value::LIST:     return "list";
        case Value::STRING:   return "string";
        default: return "???";   // Unreachable
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
        case LIST:      // same for list, might be self referential so no safe way to deep inspect
        case STRING:    // all strings are interned --> therefore can compare pointers
            return as.obj == other.as.obj;
        default: return false;   // Unreachable
    }
}

ObjString * Value::toString(Vm * vm) {
    switch( type ){
        case NIL:     return ObjString::newString(vm, "nil");
        case BOOL:    return ObjString::newString(vm, as.boolean ? "true" : "false");
        case NUMBER:  return ObjString::newStringFmt(vm, "%g", as.number);
        case TYPEID:  return ObjString::newString(vm, typeIdToString(as.typeId));
        case FUNCTION:
        case LIST:
        case STRING:
            // Object types:
            return as.obj->toString(vm);
        default:      return ObjString::newString(vm, "???");
    }
}

void Value::print() const {
    switch( type ){
        case NIL:     printf("nil"); return;
        case BOOL:    printf(as.boolean ? "true" : "false"); return;
        case NUMBER:  printf("%g", as.number); return;
        case TYPEID:  printf("%s", typeIdToString(as.typeId)); return;
        case FUNCTION:
        case LIST:
        case STRING:
            // Object types:
            as.obj->print(); return;
        default:      printf("???");
    }
}
