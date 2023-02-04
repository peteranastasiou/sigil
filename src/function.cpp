
#include "function.hpp"


ObjFunction::ObjFunction(Mem * mem, ObjString * funcName) : Obj(mem, Obj::Type::FUNCTION) {
    numInputs = 0;
    numUpvalues = 0;
    name = funcName;
}

ObjFunction::~ObjFunction() {
}

ObjString * ObjFunction::toString() {
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


ObjClosure::ObjClosure(Mem * mem, ObjFunction * func) : Obj(mem, Obj::Type::CLOSURE) {
    function = func;
}

ObjClosure::~ObjClosure() {
}

ObjString * ObjClosure::toString() {
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

ObjUpvalue::ObjUpvalue(Mem * mem, Value * val) : Obj(mem, Obj::Type::UPVALUE) {
    value = val;
}

ObjUpvalue::~ObjUpvalue() {
}

ObjString * ObjUpvalue::toString() {
    return ObjString::newString(mem_, "<upvalue>");
}

void ObjUpvalue::print(bool verbose) {
    puts("<upvalue>");
}
