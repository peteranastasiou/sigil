
#include "function.hpp"


ObjFunction::ObjFunction(Vm * vm) : Obj(vm, Obj::Type::FUNCTION) {
    arity = 0;
    name = nullptr;
}

ObjFunction::~ObjFunction() {
}
