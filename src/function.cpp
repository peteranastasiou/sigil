
#include "function.hpp"


ObjFunction::ObjFunction(Vm * vm, ObjString * funcName) : Obj(vm, Obj::Type::FUNCTION) {
    numInputs = 0;
    numUpvalues = 0;
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

// -----------------------------------------------------


ObjClosure::ObjClosure(Vm * vm, ObjFunction * func) : Obj(vm, Obj::Type::CLOSURE) {
    function = func;
}

ObjClosure::~ObjClosure() {
}

ObjString * ObjClosure::toString(Vm * vm) {
    return function->toString(vm);
}

void ObjClosure::print() {
    function->print();
}
