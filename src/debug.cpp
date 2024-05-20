
#include "debug.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "function.hpp"


Disassembler::Disassembler(){
}

Disassembler::~Disassembler(){
}

void Disassembler::disassembleChunk(Chunk * chunk, char const * name){
    printf("== %s ==\n", name);

    for( int offset = 0; offset < chunk->count(); ) {
        int line = chunk->getLineNumber(offset);
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

    uint8_t instr = chunk->code[(size_t)offset];
    switch(instr){
        case OpCode::PUSH_ZERO:     return simpleInstruction_("PUSH_ZERO");
        case OpCode::PUSH_ONE:      return simpleInstruction_("PUSH_ONE");
        case OpCode::LITERAL:       return literalInstruction_("LITERAL", chunk, offset);
        case OpCode::CLOSURE:       return closureInstruction_("CLOSURE", chunk, offset);
        case OpCode::NIL:           return simpleInstruction_("NIL");
        case OpCode::TRUE:          return simpleInstruction_("TRUE");
        case OpCode::FALSE:         return simpleInstruction_("FALSE");
        case OpCode::TYPE_BOOL:     return simpleInstruction_("TYPE_BOOL");
        case OpCode::TYPE_FLOAT:    return simpleInstruction_("TYPE_FLOAT");
        case OpCode::TYPE_FUNCTION: return simpleInstruction_("TYPE_FUNCTION");
        case OpCode::TYPE_STRING:   return simpleInstruction_("TYPE_STRING");
        case OpCode::ADD:           return simpleInstruction_("ADD");
        case OpCode::POP:           return simpleInstruction_("POP");
        case OpCode::CLOSE_UPVALUE:       return simpleInstruction_("CLOSE_UPVALUE");
        case OpCode::DEFINE_GLOBAL_VAR:   return literalInstruction_("DEFINE_GLOBAL_VAR", chunk, offset);
        case OpCode::DEFINE_GLOBAL_CONST: return literalInstruction_("DEFINE_GLOBAL_CONST", chunk, offset);
        case OpCode::GET_GLOBAL:    return byteInstruction_("GET_GLOBAL", chunk, offset);
        case OpCode::SET_GLOBAL:    return byteInstruction_("SET_GLOBAL", chunk, offset);
        case OpCode::GET_LOCAL:     return argInstruction_("GET_LOCAL", chunk, offset);
        case OpCode::SET_LOCAL:     return argInstruction_("SET_LOCAL", chunk, offset);
        case OpCode::GET_UPVALUE:   return argInstruction_("GET_UPVALUE", chunk, offset);
        case OpCode::SET_UPVALUE:   return argInstruction_("SET_UPVALUE", chunk, offset);
        case OpCode::EQUAL:         return simpleInstruction_("EQUAL");
        case OpCode::NOT_EQUAL:     return simpleInstruction_("NOT_EQUAL");
        case OpCode::GREATER:       return simpleInstruction_("GREATER");
        case OpCode::GREATER_EQUAL: return simpleInstruction_("GREATER_EQUAL");
        case OpCode::LESS:          return simpleInstruction_("LESS");
        case OpCode::LESS_EQUAL:    return simpleInstruction_("LESS_EQUAL");
        case OpCode::SUBTRACT:      return simpleInstruction_("SUBTRACT");
        case OpCode::MULTIPLY:      return simpleInstruction_("MULTIPLY");
        case OpCode::DIVIDE:        return simpleInstruction_("DIVIDE");
        case OpCode::NEGATE:        return simpleInstruction_("NEGATE");
        case OpCode::NOT:           return simpleInstruction_("NOT");
        case OpCode::COMPARE_ITERATOR:   return simpleInstruction_("COMPARE_ITERATOR");
        case OpCode::MAKE_LIST:     return argInstruction_("MAKE_LIST", chunk, offset);
        case OpCode::PRINT:         return simpleInstruction_("PRINT");
        case OpCode::ECHO:          return simpleInstruction_("ECHO");
        case OpCode::TYPE:          return simpleInstruction_("TYPE");
        case OpCode::JUMP:          return jumpInstruction_("JUMP", 1, chunk, offset);
        case OpCode::LOOP:          return jumpInstruction_("LOOP", -1, chunk, offset);
        case OpCode::JUMP_IF_TRUE:  return jumpInstruction_("JUMP_IF_TRUE", 1, chunk, offset);
        case OpCode::JUMP_IF_FALSE: return jumpInstruction_("JUMP_IF_FALSE", 1, chunk, offset);
        case OpCode::JUMP_IF_TRUE_POP: return jumpInstruction_("JUMP_IF_TRUE_POP", 1, chunk, offset);
        case OpCode::JUMP_IF_FALSE_POP: return jumpInstruction_("JUMP_IF_FALSE_POP", 1, chunk, offset);
        case OpCode::JUMP_IF_ZERO:  return jumpInstruction_("JUMP_IF_ZERO", 1, chunk, offset);
        case OpCode::CALL:          return byteInstruction_("CALL", chunk, offset);
        case OpCode::RETURN:        return simpleInstruction_("RETURN");
        default:
            printf("Unknown opcode %i\n", instr);
            return 1;
    }
}

int Disassembler::literalInstruction_(char const * name, Chunk * chunk, int offset){
    uint8_t literalIdx = chunk->code[offset + 1];
    printf("%-16s %4d ", name, literalIdx);
    chunk->literals[literalIdx].print(true);
    printf("\n");
    return 2;
}

int Disassembler::closureInstruction_(char const * name, Chunk * chunk, int offset){
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

int Disassembler::byteInstruction_(const char* name, Chunk* chunk, int offset) {
  uint8_t b = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, b);
  return 2;
}

int Disassembler::argInstruction_(char const * name, Chunk * chunk, int offset){
    uint8_t arg = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, arg);
    return 2;
}

int Disassembler::simpleInstruction_(char const * name){
    printf("%s\n", name);
    return 1;
}

int Disassembler::jumpInstruction_(const char* name, int sign, Chunk* chunk, int offset) {
    int jumpLen = (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
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
//         if( token.type == Token::END ){
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
        case Token::IDENTIFIER:     return "IDENTIFIER";
        case Token::STRING:         return "STRING";
        case Token::NUMBER:         return "NUMBER";
        case Token::AND:            return "AND";
        case Token::CONST:          return "CONST";
        case Token::ELSE:           return "ELSE";
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
        case Token::END:            return "END";
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
