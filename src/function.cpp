
#include "function.hpp"


ObjFunction::ObjFunction(Vm * vm, ObjString * funcName) : Obj(vm, Obj::Type::FUNCTION) {
    arity = 0;
    name = funcName;
}

ObjFunction::~ObjFunction() {
}

ObjString * ObjFunction::toString(Vm * vm) {
    if( name == nullptr ){
        return ObjString::newString(vm, "<script>");
    }
    return name;
}

void ObjFunction::print() {
    if( name == nullptr ){
        printf("<script>");
    }else{
        printf("<fn %s>", name->get());
    }
}
