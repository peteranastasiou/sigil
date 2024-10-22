#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "str.hpp"

#include <stdint.h>
#include <stddef.h>


class Disassembler {
public:
    Disassembler();
    ~Disassembler();

    void disassembleChunk(Chunk * chunk, char const * name);
    int disassembleInstruction(Chunk * chunk, int offset);

private:
    int disassembleInstruction_(Chunk * chunk, int offset, int line);
    int instrLiteral_(char const * name, Chunk * chunk, int offset);
    int instrClosure_(char const * name, Chunk * chunk, int offset);
    int instrArgUint8_(char const * name, Chunk * chunk, int offset);
    int instrArgInt8_(char const * name, Chunk * chunk, int offset);
    int instrSimple_(char const * name);
    int instrJump_(const char* name, int sign, Chunk* chunk, int offset);
};

#ifdef DEBUG_GC
#define debugGcPrint(...) printf(__VA_ARGS__)
#else
#define debugGcPrint(...)
#endif

// void debugScanner(char const * source);

// void printToken(Token token);
char const * tokenTypeToStr(Token::Type t);

void debugObjectLinkedList(Obj * obj);
