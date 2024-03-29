
#include "mem.hpp"
#include "vm.hpp"
#include "debug.hpp"

Mem::Mem() {
    vm_ = nullptr;
    objects_ = nullptr;
    openUpvalues_ = nullptr;
    EMPTY_STRING = nullptr;
    init_ = false;
}

Mem::~Mem() {
    freeObjects_();
}

void Mem::init(Vm * vm) {
    vm_ = vm;
    EMPTY_STRING = ObjString::newString(this, "");
    init_ = true;
}

void Mem::collectGarbage() {
    debugGcPrint("\n\nRunning garbage collector\n");

    if( !init_ ) return;

    //--------------------------------
    // MARK ROOTS
    //--------------------------------
    // Mark objects owned by vm:
    debugGcPrint( "GC Mark roots:\n" );
    vm_->gcMarkRoots();

    // Mark open upvalues:
    for( ObjUpvalue * u = openUpvalues_;
            u != nullptr; 
            u = u->getNextUpvalue() ){
        printf( "Mark upvalue:" );
        u->print(true);
        printf("\n");
        u->gcMark();
    }

    //--------------------------------
    // MARK REFERENCES
    //--------------------------------
    debugGcPrint( "\nMark references: %i\n", (int)markedObjects_.size() );
    while( markedObjects_.size() > 0 ){
        // Pop last marked object
        Obj * o = markedObjects_.back();
        markedObjects_.pop_back();

        // Mark its references 
        o->gcMarkRefs();
    }

    // Mark the empty string to keep it from being collected
    // Strings have no references so we can do this after the previous step
    EMPTY_STRING->gcMark();

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

    debugGcPrint("\nSweeping:\n");
    {
        Obj * prev = nullptr;
        Obj * obj = objects_;
        while( obj != nullptr ){

#ifdef DEBUG_GC
            printf(" %p: ", obj);
            obj->print(true);
            printf("\n");
#endif
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

#ifdef DEBUG_GC
                printf("Delete %p: ", unused);
                unused->print(true);
                printf("\n");
#endif

                delete unused;
            }
        }
    }

#ifdef DEBUG_GC
    debugGcPrint("\nPost garbage collect list of objects:\n");
    {
        Obj * prev = nullptr;
        Obj * obj = objects_;
        if( obj == nullptr ) printf("(none)\n");
        while( obj != nullptr ){

            printf(" %p: ", obj);
            obj->print(true);
            printf("\n");
            obj = obj->next;
        }
    }
#endif
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
