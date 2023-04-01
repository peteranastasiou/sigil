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

    // Mark to protect against being garbage collected
    void gcMark();

    Obj * next;  // linked list of all objects

protected:
    Mem * const mem_;
    bool isMarked;
};
