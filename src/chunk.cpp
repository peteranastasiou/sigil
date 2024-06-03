
#include "chunk.hpp"

#include <assert.h>


static int const MAX_COUNT_ = 65535;

Chunk::Chunk() {
}

Chunk::~Chunk() {
}

bool Chunk::write(uint8_t byte, uint16_t line, uint16_t frameSize) {
    if( code.size() >= MAX_COUNT_ ){
        return false;
    }
    code.push_back(byte);
    lines.push_back(line);
    predictedFrameSize.push_back(frameSize);   // DEBUG only!
    return true;
}

uint16_t Chunk::getLineNumber(int offset) {
    if( offset < 0 || offset >= (int)lines.size() ) return -1;  // should never happen
    return lines[offset];
}

uint16_t Chunk::getPredictedFrameSize(int offset) {
    assert( offset >= 0 || offset < (int)predictedFrameSize.size() );
    return predictedFrameSize[offset];
}

// TODO rename to instructionCount
int Chunk::count() {
    return (int)code.size();
}

uint8_t * Chunk::getCode() {
    return &code[0];
}

uint8_t Chunk::addLiteral(Value value) {
    // first, check if literal is already in array:
    for( int i = 0; i < (int)literals.size(); ++i ){
        if( value.equals(literals[i]) ){
            return (uint8_t)i;
        }
    }

    int size = (int)literals.size();
    if( size < MAX_LITERALS ){
        literals.push_back(value);
        return (uint8_t)size; // index of new literal
    }else{
        return MAX_LITERALS; // full!
    }
}

Value Chunk::getLiteral(uint8_t index) {
    assert(index < literals.size());
    return literals[index];
}

uint8_t Chunk::numLiterals() {
    return (uint8_t)literals.size();
}

void Chunk::gcMarkRefs() {
    for( Value & literal : literals ){
        literal.gcMark();
    }
}

int8_t Chunk::frameImpact(OpCode op, uint8_t arg) {
    switch( op ){
        // Net impact of one more value on the stack
        case OpCode::PUSH_ZERO:
        case OpCode::PUSH_ONE:
        case OpCode::PUSH_TWO:
        case OpCode::LITERAL:
        case OpCode::CLOSURE:
        case OpCode::NIL:
        case OpCode::TRUE:
        case OpCode::FALSE:
        case OpCode::TYPE_BOOL:
        case OpCode::TYPE_FLOAT:
        case OpCode::TYPE_FUNCTION:
        case OpCode::TYPE_STRING:
        case OpCode::TYPE_TYPEID:
        case OpCode::GET_GLOBAL:
        case OpCode::GET_LOCAL:
        case OpCode::GET_UPVALUE:
        case OpCode::COMPARE_ITERATOR:
        case OpCode::INDEX_GET:
            return 1;

        // Net neutral impact on the stack
        case OpCode::SET_GLOBAL:
        case OpCode::SET_LOCAL:
        case OpCode::SET_UPVALUE:
        case OpCode::NEGATE:
        case OpCode::NOT:
        case OpCode::PRINT:
        case OpCode::ECHO:
        case OpCode::TYPE:
        case OpCode::INDEX_SET:
        case OpCode::JUMP:
        case OpCode::LOOP:
        case OpCode::JUMP_IF_TRUE:
        case OpCode::JUMP_IF_FALSE:
        case OpCode::JUMP_IF_ZERO:
            return 0;

        // Net impact of one less value on the stack
        case OpCode::POP:
        case OpCode::DEFINE_GLOBAL_VAR:
        case OpCode::DEFINE_GLOBAL_CONST:
        case OpCode::APPEND_LOCAL:
        case OpCode::CLOSE_UPVALUE:
        case OpCode::EQUAL:
        case OpCode::NOT_EQUAL:
        case OpCode::GREATER:
        case OpCode::GREATER_EQUAL:
        case OpCode::LESS:
        case OpCode::LESS_EQUAL:
        case OpCode::ADD:
        case OpCode::SUBTRACT:
        case OpCode::MULTIPLY:
        case OpCode::DIVIDE:
        case OpCode::JUMP_IF_TRUE_POP:
        case OpCode::JUMP_IF_FALSE_POP:
            return -1;

        // Special cases:
        case OpCode::MAKE_LIST:
        case OpCode::CALL:
            // consumes `arg` number of values
            // and pushes one result
            return (int8_t)(1 - arg);

        // Syntactically speaking only, return removes 1 value
        case OpCode::RETURN:
            return -1;
    }
    // Shouldn't happen
    return 0;
}
