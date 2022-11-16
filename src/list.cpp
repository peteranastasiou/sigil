
#include "list.hpp"
#include "str.hpp"
#include <string.h>

ObjList::ObjList(Vm * vm) : Obj(vm, Obj::LIST) {
}

ObjList::~ObjList() {
}

ObjString * ObjList::toString(Vm * vm) {
    return ObjString::newString(vm, "<list>");
}

void ObjList::print() {
    printf("<list>");
}

void ObjList::append(Value v) {
    values.push_back(v);
}

bool ObjList::get(int i, Value & v) {
    // python-style count from the back
    if( i < 0 ) i = len() + i;

    // check out of bounds:
    if( i < 0 || i >= len() ){
        return false;
    }
    v = values.at(i);
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
        values.reserve(newLen);
        memset(&values[firstNewIdx], 0, numValues*sizeof(Value));
    }
    v = values.at(i);
    return true;
}

int ObjList::len() {
    return (int)values.size();
}
