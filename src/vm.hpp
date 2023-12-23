#pragma once

#include "mem.hpp"
#include "chunk.hpp"
#include "value.hpp"
#include "object.hpp"
#include "table.hpp"
#include "inputstream/inputstream.hpp"

#include <unordered_map>

// Predeclare compiler
class Compiler;

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

    ObjClosure * closure;
    uint8_t * ip;   // instruction pointer
    Value * slots;  // first value in stack which can be used by function
};

struct Global {
    Value value;
    bool isConst;

    //  Implement gc mark to use as hashmap element:
    void gcMark() {
        value.gcMark();
    }
};

class Vm {
public:
    Vm();
    ~Vm();

    void init();

    InterpretResult interpret(char const * name, InputStream * stream);

    // Mark root objects to preserve from garbage collection:
    void gcMarkRoots();

    // stack operations:
    void push(Value value);
    Value pop();
    void pop(int n);
    Value peek(int index);  // index counts from top (end) of stack

private:
    void resetStack_();
    InterpretResult run_();
    bool call_(ObjClosure * fn, uint8_t argCount);
    bool callValue_(Value value, uint8_t argCount);
    bool binaryOp_(uint8_t op);
    bool isTruthy_(Value value);
    void concatenate_();
    bool indexGet_();
    InterpretResult runtimeError_(const char* format, ...);

    static int const FRAMES_MAX = 64;
    static int const STACK_MAX = FRAMES_MAX * 256;

    Mem mem_;
    Compiler * compiler_;
    CallFrame frames_[FRAMES_MAX];  // TODO to allow continuations/generators, this can't be a stack, GC instead
    int frameCount_;
    Value stack_[STACK_MAX];
    Value * stackTop_;  // points past the last value in the stack
    HashMap<Global> globals_; 
};
