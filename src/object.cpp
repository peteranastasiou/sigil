
#include "object.hpp"
#include "mem.hpp"


Obj::Obj(Mem * mem, Type t): type(t), mem_(mem) {
    mem_->registerObj(this);
}

Obj::~Obj(){
    mem_->deregisterObj(this);
}
