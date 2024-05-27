
#include "function.hpp"
#include "upvalue.hpp"

ObjFunction::ObjFunction(Mem * mem, ObjString * funcName) : Obj(mem) {
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
        printf("%s", name->get());
    }
}

void ObjFunction::gcMarkRefs() {
    name->gcMark();
    chunk.gcMarkRefs();
}

// -----------------------------------------------------


ObjClosure::ObjClosure(Mem * mem, ObjFunction * func) : Obj(mem) {
    function = func;
}

ObjClosure::~ObjClosure() {
}

ObjString * ObjClosure::toString() {
    return function->name;
}

void ObjClosure::print(bool verbose) {
    printf("%s", function->name->get());
}

void ObjClosure::gcMarkRefs() {
    function->gcMark();
    for( ObjUpvalue * u : upvalues ){
        u->gcMark();
    }
}