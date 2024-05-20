
#include "list.hpp"
#include "str.hpp"
#include <string.h>

ObjList::ObjList(Mem * mem) : Obj(mem) {
}

ObjList::~ObjList() {
}

ObjString * ObjList::toString() {
    return ObjString::newString(mem_, "<list>");
}

void ObjList::print(bool verbose) {
    (void) verbose;
    if( len() == 0 ){
        printf("[]");
        return;
    }

    printf("[");
    for( int i = 0; i < len()-1; ++i ){
        values_[i].print(true);
        printf(", ");
    }
    values_[len()-1].print(true);
    printf("]");
}

void ObjList::gcMarkRefs() {
    for( Value & value : values_ ){
        value.gcMark();
    }
}

void ObjList::concat(ObjList * a) {
    values_.insert(values_.end(), a->values_.begin(), a->values_.end());
}

void ObjList::append(Value v) {
    values_.push_back(v);
}

bool ObjList::get(int i, Value & v) {
    // python-style count from the back
    if( i < 0 ) i = len() + i;

    // check out of bounds:
    if( i < 0 || i >= len() ) return false;

    v = values_[i];
    return true;
}

bool ObjList::set(int i, Value v) {
    // python-style count from the back
    if( i < 0 ) i = len() + i;

    // check out of bounds:
    if( i < 0 ) return false;

    // check if need to grow the list:
    if( i >= len() ){
        int newLen = i + 1;
        int firstNewIdx = len();
        int numValues = newLen - len();

        values_.resize(newLen);

        // filling with zeroes is equivalent to filling with Nil values
        memset(&values_[firstNewIdx], 0, numValues*sizeof(Value));
    }
    values_[i] = v;
    return true;
}

int ObjList::len() {
    return (int)values_.size();
}
