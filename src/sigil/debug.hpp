#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "str.hpp"

#include <stdint.h>
#include <stddef.h>


class Disassembler {
public:
    static void disassembleChunk(Chunk * chunk, char const * name);
    static int disassembleInstruction(Chunk * chunk, int offset);

private:
    static int disassembleInstruction_(Chunk * chunk, int offset, int line);
    static int instrLiteral_(char const * name, Chunk * chunk, int offset);
    static int instrClosure_(char const * name, Chunk * chunk, int offset);
    static int instrArgUint8_(char const * name, Chunk * chunk, int offset);
    static int instrArgInt8_(char const * name, Chunk * chunk, int offset);
    static int instrSimple_(char const * name);
    static int instrJump_(const char* name, int sign, Chunk* chunk, int offset);
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
