
#include "value.hpp"

#include <stdio.h>

static char const* typeIdToString(Value::Type t) {
    switch( t ){
        case Value::NIL:     return "nil";
        case Value::BOOL:    return "bool";
        case Value::NUMBER:  return "float";
        case Value::OBJECT:  return "object";
        case Value::STRING:  return "string";
        case Value::TYPEID:  return "typeid";
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
        case OBJECT:  return false;  // TODO
        case STRING:{
            if( other.type != STRING ) return false;
            // all strings are interned --> therefore can compare pointers
            return asObjString() == other.asObjString();
        }
        default: return false;   // Unreachable
    }
}

ObjString * Value::toString(Vm * vm) {
    switch( type ){
        case NIL:     return ObjString::newString(vm, "nil");
        case BOOL:    return ObjString::newString(vm, as.boolean ? "true" : "false");
        case NUMBER:  return ObjString::newStringFmt(vm, "%g", as.number);
        case OBJECT:  // fall-through:
        case STRING:  return as.obj->toString();
        case TYPEID:  return ObjString::newString(vm, typeIdToString(as.typeId));
        default:      return ObjString::newString(vm, "???");
    }
}

void Value::print() const {
    switch( type ){
        case NIL:     printf("nil"); return;
        case BOOL:    printf(as.boolean ? "true" : "false"); return;
        case NUMBER:  printf("%g", as.number); return;
        case OBJECT:  // fall-through:
        case STRING:  as.obj->print(); return;
        case TYPEID:  printf("%s", typeIdToString(as.typeId)); return;
        default:      printf("???");
    }
}
