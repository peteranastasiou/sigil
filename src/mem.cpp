
#include "mem.hpp"
#include "vm.hpp"

// TODO turn off by default
#define DEBUG_STRESS_GC

Mem::Mem(Vm * vm) {
    vm_ = vm;
    objects_ = nullptr;
    openUpvalues_ = nullptr;
}

Mem::~Mem() {
    freeObjects_();
}

void Mem::collectGarbage() {
    // Mark and sweep
    vm_->gcMarkRoots();
}

void Mem::registerObj(Obj * obj) {
    // Creating a new object, so perhaps run garbage collector now
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

    // Add to linked list of objects
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
