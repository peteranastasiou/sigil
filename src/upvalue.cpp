
#include "upvalue.hpp"
#include "mem.hpp"
#include "str.hpp"


ObjUpvalue * ObjUpvalue::newUpvalue(Mem * mem, Value * value) {
    // Iterate linked list looking for upvalue pointing to the local:
    ObjUpvalue * curr = mem->getRootOpenUpvalue();
    ObjUpvalue * prev = nullptr;
    // stop searching when we get to the end of the upvalue list
    // or "after" the local we are looking for (list is sorted)
    while( curr != nullptr && curr->value_ > value ) {
        if( curr->value_ == value ){
            // already exists:
            return curr;
        }
        prev = curr;
        curr = curr->nextUpvalue_;
    }
    // not found: create upvalue
    ObjUpvalue * upvalue = new ObjUpvalue(mem, value);

    // wire into the linked list:
    upvalue->next = curr;
    if( prev == nullptr ){
        mem->setRootOpenUpvalue(upvalue);
    }else{
        prev->next = upvalue;
    }
    return upvalue;
}

ObjUpvalue::ObjUpvalue(Mem * mem, Value * val) : Obj(mem) {
    value_ = val;
    closedValue_ = Value::nil();
}

ObjUpvalue::~ObjUpvalue() {
}

void ObjUpvalue::close() {
    // Shift the value from the stack to internal value:
    closedValue_ = *value_;

    // From now on, use the internal value:
    value_ = &closedValue_;
}
ObjString * ObjUpvalue::toString() {

    return ObjString::newString(mem_, "<upvalue>");
}

void ObjUpvalue::print(bool verbose) {
    if( verbose ){
        printf("<upvalue %s ",
            value_ == &closedValue_ ? "closed" : "open");
        value_->print(true);
        printf(">");
    }else{
        printf("<upvalue>");
    }
}

void ObjUpvalue::gcMarkRefs() {
    closedValue_.gcMark();
}