
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

void Mem::closeUpvalues(Value * stackTop){
    // This function is called when the stack shrinks, and upvalues pointing to values
    // that just got popped of the stack should be closed.

    // Note: the newest upvalue is at the head of the list
    // Starting from the head, close upvalues that are off the end of the stackTop
    while( openUpvalues_ != nullptr && openUpvalues_->ref() >= stackTop ){
        // close the first upvalue, and shift the head along
        openUpvalues_->close();
        openUpvalues_ = openUpvalues_->getNextUpvalue();
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
