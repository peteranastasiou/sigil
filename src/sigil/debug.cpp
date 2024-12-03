
#include "debug.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "function.hpp"
#include "encode.hpp"


void Disassembler::disassembleChunk(Chunk * chunk, char const * name){
    printf("== %s ==\n", name);

    int lastLine = -1;

    for( int offset = 0; offset < chunk->count(); ) {
        int line = chunk->getLineNumber(offset);
        if( line != lastLine ) {
            printf("LINE %i:\n", line); // refer newCopyOfStream
        }
        lastLine = line;
        int incr = disassembleInstruction_(chunk, offset, line);
        offset += incr;
    }
}

int Disassembler::disassembleInstruction(Chunk * chunk, int offset){
    int line = chunk->getLineNumber(offset);
    return disassembleInstruction_(chunk, offset, line);
}

int Disassembler::disassembleInstruction_(Chunk * chunk, int offset, int line){
    printf("%04i ", offset);
    printf("%4d ", line);

    OpCode instr = (OpCode)chunk->code[(size_t)offset];
    switch(instr){
        case OpCode::PUSH_ZERO:     return instrSimple_("PUSH_ZERO");
        case OpCode::PUSH_ONE:      return instrSimple_("PUSH_ONE");
        case OpCode::PUSH_TWO:      return instrSimple_("PUSH_TWO");
        case OpCode::LITERAL:       return instrLiteral_("LITERAL", chunk, offset);
        case OpCode::CLOSURE:       return instrClosure_("CLOSURE", chunk, offset);
        case OpCode::NIL:           return instrSimple_("NIL");
        case OpCode::END:           return instrSimple_("END");
        case OpCode::TRUE:          return instrSimple_("TRUE");
        case OpCode::FALSE:         return instrSimple_("FALSE");
        case OpCode::TYPE_BOOL:     return instrSimple_("TYPE_BOOL");
        case OpCode::TYPE_FLOAT:    return instrSimple_("TYPE_FLOAT");
        case OpCode::TYPE_FUNCTION: return instrSimple_("TYPE_FUNCTION");
        case OpCode::TYPE_STRING:   return instrSimple_("TYPE_STRING");
        case OpCode::ADD:           return instrSimple_("ADD");
        case OpCode::POP:           return instrSimple_("POP");
        case OpCode::CLOSE_UPVALUE:       return instrSimple_("CLOSE_UPVALUE");
        case OpCode::DEFINE_GLOBAL_VAR:   return instrLiteral_("DEFINE_GLOBAL_VAR", chunk, offset);
        case OpCode::DEFINE_GLOBAL_CONST: return instrLiteral_("DEFINE_GLOBAL_CONST", chunk, offset);
        case OpCode::GET_GLOBAL:    return instrArgUint8_("GET_GLOBAL", chunk, offset);
        case OpCode::SET_GLOBAL:    return instrArgUint8_("SET_GLOBAL", chunk, offset);
        case OpCode::GET_LOCAL:     return instrArgInt8_("GET_LOCAL", chunk, offset);
        case OpCode::SET_LOCAL:     return instrArgInt8_("SET_LOCAL", chunk, offset);
        case OpCode::APPEND_LOCAL:  return instrArgInt8_("APPEND_LOCAL", chunk, offset);
        case OpCode::GET_UPVALUE:   return instrArgUint8_("GET_UPVALUE", chunk, offset);
        case OpCode::SET_UPVALUE:   return instrArgUint8_("SET_UPVALUE", chunk, offset);
        case OpCode::EQUAL:         return instrSimple_("EQUAL");
        case OpCode::NOT_EQUAL:     return instrSimple_("NOT_EQUAL");
        case OpCode::GREATER:       return instrSimple_("GREATER");
        case OpCode::GREATER_EQUAL: return instrSimple_("GREATER_EQUAL");
        case OpCode::LESS:          return instrSimple_("LESS");
        case OpCode::LESS_EQUAL:    return instrSimple_("LESS_EQUAL");
        case OpCode::SUBTRACT:      return instrSimple_("SUBTRACT");
        case OpCode::MULTIPLY:      return instrSimple_("MULTIPLY");
        case OpCode::DIVIDE:        return instrSimple_("DIVIDE");
        case OpCode::NEGATE:        return instrSimple_("NEGATE");
        case OpCode::NOT:           return instrSimple_("NOT");
        case OpCode::COMPARE_ITERATOR:   return instrSimple_("COMPARE_ITERATOR");
        case OpCode::MAKE_LIST:     return instrArgUint8_("MAKE_LIST", chunk, offset);
        case OpCode::PRINT:         return instrSimple_("PRINT");
        case OpCode::ECHO:          return instrSimple_("ECHO");
        case OpCode::TYPE:          return instrSimple_("TYPE");
        case OpCode::JUMP:          return instrJump_("JUMP", 1, chunk, offset);
        case OpCode::JUMP_IF_TRUE:  return instrJump_("JUMP_IF_TRUE", 1, chunk, offset);
        case OpCode::JUMP_IF_FALSE: return instrJump_("JUMP_IF_FALSE", 1, chunk, offset);
        case OpCode::JUMP_IF_TRUE_POP: return instrJump_("JUMP_IF_TRUE_POP", 1, chunk, offset);
        case OpCode::JUMP_IF_FALSE_POP: return instrJump_("JUMP_IF_FALSE_POP", 1, chunk, offset);
        case OpCode::JUMP_IF_ZERO:  return instrJump_("JUMP_IF_ZERO", 1, chunk, offset);
        case OpCode::CALL:          return instrArgUint8_("CALL", chunk, offset);
        case OpCode::RETURN:        return instrSimple_("RETURN");
        default:
            printf("Unknown opcode %i\n", (int)instr);
            return 1;
    }
}

int Disassembler::instrLiteral_(char const * name, Chunk * chunk, int offset){
    uint8_t literalIdx = chunk->code[offset + 1];
    printf("%-16s %4d ", name, literalIdx);
    chunk->literals[literalIdx].print(true);
    printf("\n");
    return 2;
}

int Disassembler::instrClosure_(char const * name, Chunk * chunk, int offset){
    int initialOffset = offset;
    offset ++;
    uint8_t literalIdx = chunk->code[offset++];
    printf("%-16s %4d ", name, literalIdx);
    chunk->literals[literalIdx].print(true);
    printf("\n");

    ObjFunction* fn = chunk->literals[literalIdx].asObjFunction();
    for (int j = 0; j < fn->numUpvalues; j++) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf("%04d      |                     %s %d\n",
                offset - 2, isLocal ? "local" : "upvalue", index);
    }

    return offset - initialOffset;
}

int Disassembler::instrArgUint8_(const char* name, Chunk* chunk, int offset) {
    uint8_t b = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, b);
    return 2;
}

int Disassembler::instrArgInt8_(char const * name, Chunk * chunk, int offset){
    int8_t arg = (int8_t)(chunk->code[offset + 1]);
    printf("%-16s %4d\n", name, arg);
    return 2;
}

int Disassembler::instrSimple_(char const * name){
    printf("%s\n", name);
    return 1;
}

int Disassembler::instrJump_(const char* name, int sign, Chunk* chunk, int offset) {
    int jumpLen = encode::unpackInt16(chunk->code[offset + 1], chunk->code[offset + 2]);
    printf("%-16s %4d -> %d\n", name, offset,
            offset + 3 + sign * jumpLen);
    return 3;
}

// void debugScanner(char const * source) {
//     Scanner scanner;
//     scanner.init(source);
//     int line = -1;
//     for(;;){
//         Token token = scanner.scanToken();
//         if( token.line != line ){
//             printf("%4d ", token.line);
//             line = token.line;
//         }else{
//             printf("   | ");
//         }
//         printToken(token);
//         printf("\n");
//         if( token.type == Token::FILE_END ){
//             break;
//         }
//     }
// }

// void printToken(Token token) {
//     printf("%s '%.*s'", tokenTypeToStr(token.type), token.length, token.start);
// }

char const * tokenTypeToStr(Token::Type t) {
    switch(t) {
        case Token::LEFT_PAREN:     return "LEFT_PAREN";
        case Token::RIGHT_PAREN:    return "RIGHT_PAREN";
        case Token::LEFT_BRACE:     return "LEFT_BRACE";
        case Token::RIGHT_BRACE:    return "RIGHT_BRACE";
        case Token::COMMA:          return "COMMA";
        case Token::MINUS:          return "MINUS";
        case Token::PLUS:           return "PLUS";
        case Token::SEMICOLON:      return "SEMICOLON";
        case Token::SLASH:          return "SLASH";
        case Token::STAR:           return "STAR";
        case Token::BANG:           return "BANG";
        case Token::BANG_EQUAL:     return "BANG_EQUAL";
        case Token::EQUAL:          return "EQUAL";
        case Token::EQUAL_EQUAL:    return "EQUAL_EQUAL";
        case Token::GREATER:        return "GREATER";
        case Token::GREATER_EQUAL:  return "GREATER_EQUAL";
        case Token::LESS:           return "LESS";
        case Token::LESS_EQUAL:     return "LESS_EQUAL";
        case Token::ARROW:          return "ARROW";
        case Token::IDENTIFIER:     return "IDENTIFIER";
        case Token::STRING:         return "STRING";
        case Token::NUMBER:         return "NUMBER";
        case Token::AND:            return "AND";
        case Token::CONST:          return "CONST";
        case Token::ELSE:           return "ELSE";
        case Token::END:            return "END";
        case Token::FALSE:          return "FALSE";
        case Token::FOR:            return "FOR";
        case Token::FN:             return "FN";
        case Token::IF:             return "IF";
        case Token::NIL:            return "NIL";
        case Token::OR:             return "OR";
        case Token::PRINT:          return "PRINT";
        case Token::RETURN:         return "RETURN";
        case Token::TRUE:           return "TRUE";
        case Token::TYPE:           return "TYPE";
        case Token::VAR:            return "VAR";
        case Token::WHILE:          return "WHILE";
        case Token::ERROR:          return "ERROR";
        case Token::FILE_END:       return "FILE_END";
        default:                    return "UNIDENTIFIED";
    }
}


void debugObjectLinkedList(Obj * obj) {
    printf("Objects:\n");
    while( obj != nullptr ){
        printf("  %p: ", obj);
        obj->print(true);
        printf("\n");
        obj = obj->next;
    }
}
