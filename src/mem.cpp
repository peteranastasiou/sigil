
#include "mem.hpp"

Mem::Mem() {
    objects_ = nullptr;
    openUpvalues_ = nullptr;
}

Mem::~Mem() {
    freeObjects_();
}

void Mem::registerObj(Obj * obj) {
    obj->next = objects_;  // previous head
    objects_ = obj;        // new head
}

void Mem::deregisterObj(Obj * obj){
    // TODO
}

void Mem::closeUpvalues(Value * last){
    // Close all open upvalues from the head of the list up to and including the `last` value
    while( openUpvalues_ != nullptr && openUpvalues_->value_ >= last ){
        // Get first upvalue
        ObjUpvalue * upvalue = openUpvalues_;

        // Shift the value from the stack to the upvalue itself
        upvalue->closedValue_ = *upvalue->value_;

        // Point the upvalue to now use its own value
        upvalue->value_ = &upvalue->closedValue_;

        // Update the head of the list to the new head
        openUpvalues_ = upvalue->nextUpvalue_;
    }
}

void Mem::freeObjects_() {
    // iterate linked list of objects, deleting them
    Obj * obj = objects_;
    while( obj != nullptr ){
        Obj * next  = obj->next;
        delete obj;
        obj = next;
    }
}
