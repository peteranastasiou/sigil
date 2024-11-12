
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
    const char * n = name->get();
    if( n[0] == '\0' ) {
        n = "{anon}";
    }
    if( verbose ){
        printf("<fn:%s>", n);
    }else{
        puts(n);
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
    const char * n = function->name->get();
    if( n[0] == '\0' ) {
        n = "{anon}";
    }
    if( verbose ){
        printf("<cl:%s>", n);
    }else{
        puts(n);
    }
}

void ObjClosure::gcMarkRefs() {
    function->gcMark();
    for( ObjUpvalue * u : upvalues ){
        u->gcMark();
    }
}