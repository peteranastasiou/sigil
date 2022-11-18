#pragma once

#include "value.hpp"

#include <stdint.h>
#include <stddef.h>
#include <vector>

namespace OpCode {
enum {
    // Literals:
    LITERAL,        // Push a literal value from the chunk
    NIL,            // Push nil to the stack
    TRUE,           // Push true to the stack
    FALSE,          // Push false to the stack
    TYPE_BOOL,      // TypeId of bool
    TYPE_FLOAT,     // TypeId of float
    TYPE_FUNCTION,  // TypeId of Object
    TYPE_STRING,    // TypeId of String
    TYPE_TYPEID,    // TypeId of TypeId
    // Stack and variable manipulation
    POP,            // Pop 1 value from the stack
    POP_N,          // Pop N values from the stack
    DEFINE_GLOBAL_VAR,   // Define a global variable
    DEFINE_GLOBAL_CONST, // Define a global variable as const
    GET_GLOBAL,     // Push the value of a global to the stack
    SET_GLOBAL,     // Set the value of a variable
    GET_LOCAL,
    SET_LOCAL,
    // Binary operators: take two values from the stack and push one:
    EQUAL,
    NOT_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    // Unary operators: take one value, push one value:
    NEGATE,
    NOT,
    // Built-ins:
    PRINT,              // Pop 1 value, print it, Push nil
    TYPE,               // Pop 1 value, Push 1 typeid
    MAKE_LIST,          // Pop n values into a list, Push list
    // Control flow:
    JUMP,               // Unconditionally jump forward by bytecode offset 
    LOOP,               // Unconditionally jump backwards by bytecode offset 
    JUMP_IF_TRUE,       // If top of stack is truthy, jump fwd by bytecode offset
    JUMP_IF_FALSE,      // If top of stack is falsy, jump fwd by bytecode offset
    JUMP_IF_TRUE_POP,   // Same as JUMP_IF_FALSE, but also pops the value
    JUMP_IF_FALSE_POP,  // Same as JUMP_IF_TRUE, but also pops the value
    CALL,               // call function
    RETURN,
};
}

struct LineNum {
    uint16_t line;  // line number
    uint8_t count;  // number of instructions on the line
};

class Chunk {
public:
    Chunk();

    ~Chunk();

    // append to bytecode array
    void write(uint8_t byte, uint16_t line);
    
    // Get a line number corresponding to position in bytecode array
    uint16_t getLineNumber(int offset);

    // Get the length of the bytecode array
    int count();

    // Get a pointer to the bytecode array
    uint8_t * getCode();

    // Add a literal value and return its index
    uint8_t addLiteral(Value value);

    // Get a literal value by its index
    Value getLiteral(uint8_t index);

    uint8_t numLiterals();

    static uint8_t const MAX_LITERALS = 255;  // literal index must fit in a byte (for now)

private:
    std::vector<uint8_t> code;
    std::vector<uint16_t> lines;    // line numbers corresponding to bytecode array
    std::vector<Value> literals;

    // Disassembler needs access within the chunk:
    friend class Disassembler;
};

