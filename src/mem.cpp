
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
    //--------------------------------
    // MARK ROOTS
    //--------------------------------
    // Mark objects owned by vm:
    vm_->gcMarkRoots();

    // Mark open upvalues:
    for( ObjUpvalue * u = openUpvalues_;
            u != nullptr; 
            u = u->getNextUpvalue() ){
        u->gcMark();
    }

    //--------------------------------
    // MARK REFERENCES
    //--------------------------------
    while( markedObjects_.size() > 0 ){
        // Pop last marked object
        Obj * o = markedObjects_.back();
        markedObjects_.pop_back();

        // Mark its references 
        o->gcMarkRefs();
    }

    //----------------------------------
    // REMOVE UNMARKED INTERNED STRINGS
    //----------------------------------
    // The interned string set is a special case.
    // We might be about to delete some strings,
    // but that could leave dangling pointers in
    // the set. So we sweep through and remove them
    // first (before isMarked gets cleared):
    internedStrings_.gcSweep();

    //----------------------------------
    // SWEEP UNMARKED OBJECTS
    //----------------------------------
    Obj * prev = nullptr;
    Obj * obj = objects_;
    while( obj != nullptr ){
        if( obj->isMarked ){
            // Clear isMarked for next GC cycle:
            obj->isMarked = false;
            prev = obj;
            obj = obj->next;
        }else{
            // Unused object, prepare for deletion:
            Obj * unused = obj;
            obj = obj->next;
            // Wire it out of the linked list:
            if( prev == nullptr ){
                objects_ = obj;  // new head
            }else{
                prev->next = obj; // bypass unused obj
            }
            delete unused;
        }
    }
}

void Mem::addGrayObj(Obj * obj) {
    markedObjects_.push_back(obj);
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
