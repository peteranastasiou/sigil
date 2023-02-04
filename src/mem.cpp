
#include "mem.hpp"

Mem::Mem() {
    objects_ = nullptr;
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

void Mem::freeObjects_() {
    // iterate linked list of objects, deleting them
    Obj * obj = objects_;
    while( obj != nullptr ){
        Obj * next  = obj->next;
        delete obj;
        obj = next;
    }
}
