
#include "function.hpp"


ObjFunction::ObjFunction(Vm * vm, ObjString * funcName) : Obj(vm, Obj::Type::FUNCTION) {
    numInputs = 0;
    numUpvalues = 0;
    name = funcName;
}

ObjFunction::~ObjFunction() {
}

ObjString * ObjFunction::toString(Vm * vm) {
    return name;
}

void ObjFunction::print(bool verbose) {
    if( verbose ){
        printf("<fn:%s>", name->get());
    }else{
        puts(name->get());
    }
}

// -----------------------------------------------------


ObjClosure::ObjClosure(Vm * vm, ObjFunction * func) : Obj(vm, Obj::Type::CLOSURE) {
    function = func;
}

ObjClosure::~ObjClosure() {
}

ObjString * ObjClosure::toString(Vm * vm) {
    return function->name;
}

void ObjClosure::print(bool verbose) {
    if( verbose ){
        printf("<cl:%s>", function->name->get());
    }else{
        puts(function->name->get());
    }
}

// -----------------------------------------------------

ObjUpvalue::ObjUpvalue(Vm * vm, Value * val) : Obj(vm, Obj::Type::UPVALUE) {
    value = val;
}

ObjUpvalue::~ObjUpvalue() {
}

ObjString * ObjUpvalue::toString(Vm * vm) {
    return ObjString::newString(vm, "<upvalue>");
}

void ObjUpvalue::print(bool verbose) {
    puts("<upvalue>");
}
