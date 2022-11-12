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
    int literalInstruction_(char const * name, Chunk * chunk, int offset);
    int byteInstruction_(char const * name, Chunk * chunk, int offset);
    int argInstruction_(char const * name, Chunk * chunk, int offset);
    int simpleInstruction_(char const * name);
    int jumpInstruction_(const char* name, int sign, Chunk* chunk, int offset);
};

void debugScanner(char const * source);

void printToken(Token token);
char const * tokenTypeToStr(Token::Type t);

void debugObjectLinkedList(Obj * obj);
