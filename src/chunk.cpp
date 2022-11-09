
#include "chunk.hpp"

#include <stdlib.h>  // exit
#include <assert.h>


static int const MAX_COUNT_ = 65535;

Chunk::Chunk() {
}

Chunk::~Chunk() {
}

void Chunk::write(uint8_t byte, uint16_t line) {
    if( code.size() >= MAX_COUNT_ ){
        // TODO fatal error
        exit(1);
    }
    code.push_back(byte);
    lines.push_back(line);
}

uint16_t Chunk::getLineNumber(int offset) {
    if( offset < 0 || offset >= (int)lines.size() ) return -1;  // should never happen
    return lines[offset];
}

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
