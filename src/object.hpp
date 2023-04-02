#pragma once

// Predeclare references
class Mem;
class ObjString;  // defined in str.hpp


class Obj {
public:
    Obj(Mem * mem);

    virtual ~Obj();

    virtual ObjString * toString() = 0;
    virtual void print(bool verbose) = 0;

    // Marking protects the object being garbage collected
    void gcMark();

    // Mark references to other objects from this one
    virtual void gcMarkRefs() = 0;

    Obj * next;  // linked list of all objects
    bool isMarked;  // used by GC to track whether object is in use

protected:
    Mem * const mem_;
};
