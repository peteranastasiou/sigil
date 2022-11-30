#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "object.hpp"
#include "table.hpp"

#include <unordered_map>

enum class InterpretResult {
    OK,
    COMPILE_ERR,
    RUNTIME_ERR
};

struct CallFrame {
    inline uint8_t readByte() { return *ip++; }
    uint16_t readUint16();
    Value readLiteral();
    ObjString * readString();
    int chunkOffsetOf(uint8_t * addr);  // instruction address to chunk offset

    ObjFunction * function;
    uint8_t * ip;   // instruction pointer
    Value * slots;  // first value in stack which can be used by function
};

struct Global {
    Value value;
    bool isConst;

    void print() const {  // required for hashmap value
        value.print();
    }
};

class Vm {
public:
    Vm();
    
    ~Vm();

    InterpretResult interpret(char const * source);

    // stack operations:
    void push(Value value);
    Value pop();
    void pop(int n);
    Value peek(int index);  // index counts from top (end) of stack

    // adding/removing objects, called from Obj(), ~Obj()
    void registerObj(Obj * obj);
    void deregisterObj(Obj * obj);

    // intern string helper
    StringSet * getInternedStrings(){ return &internedStrings_; }

private:
    void resetStack_();
    InterpretResult run_();
    bool call_(ObjFunction * fn, uint8_t argCount);
    bool callValue_(Value value, uint8_t argCount);
    bool binaryOp_(uint8_t op);
    bool isTruthy_(Value value);
    void concatenate_();
    bool indexGet_();
    InterpretResult runtimeError_(const char* format, ...);
    void freeObjects_();

    static int const FRAMES_MAX = 64;
    static int const STACK_MAX = FRAMES_MAX * 256;

    CallFrame frames_[FRAMES_MAX];  // TODO to allow continuations/generators, this can't be a stack, GC instead
    int frameCount_;
    Value stack_[STACK_MAX];
    Value * stackTop_;  // points past the last value in the stack
    Obj * objects_;     // linked list of objects
    StringSet internedStrings_;
    HashMap<Global> globals_; 
};
