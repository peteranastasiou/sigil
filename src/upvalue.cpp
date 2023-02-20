
#include "upvalue.hpp"
#include "str.hpp"


ObjUpvalue::ObjUpvalue(Mem * mem, Value * val) : Obj(mem) {
    value_ = val;
}

ObjUpvalue::~ObjUpvalue() {
}

ObjString * ObjUpvalue::toString() {
    return ObjString::newString(mem_, "<upvalue>");
}

void ObjUpvalue::print(bool verbose) {
    puts("<upvalue>");
}
