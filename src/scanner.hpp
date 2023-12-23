#pragma once

#include "inputstream/inputstream.hpp"
#include "mem.hpp"
#include <stdint.h>

struct Token {
    enum Type : uint8_t {
        // Single-character tokens:
        LEFT_PAREN, RIGHT_PAREN,      // ()
        LEFT_BRACE, RIGHT_BRACE,      // {}
        LEFT_BRACKET, RIGHT_BRACKET,  // []
        COMMA, MINUS, PLUS,
        SEMICOLON, SLASH, STAR,
        DOT,
        // One or more character tokens:
        BANG, BANG_EQUAL,
        EQUAL, EQUAL_EQUAL,
        GREATER, GREATER_EQUAL,
        LESS, LESS_EQUAL,
        COLON, COLON_EQUAL,
        // Literals:
        IDENTIFIER, STRING, NUMBER,   // TODO int
        // Keywords:
        AND, BOOL, CONST, ELIF, ELSE, FALSE,
        FOR, FN, FLOAT, IF, IN, NIL, OR, OBJECT,
        PRINT, ECHO, RETURN, STRING_TYPE,
        TRUE, TYPE, TYPEID, VAR, WHILE,
        // Special tokens:
        ERROR, END
    };

    Type type;
    uint16_t line;
    uint16_t col;
    ObjString * string;

    Token(): string(nullptr) {}
    Token(Type t, uint16_t l, uint16_t c): type(t), line(l), col(c), string(nullptr) {}
    Token(Type t, uint16_t l, uint16_t c, ObjString * s): type(t), line(l), col(c), string(s) {}
};

class Scanner {
public:
    Scanner();
    
    ~Scanner();

    void init(Mem * mem, InputStream * stream);

    Token scanToken();

    char const * getPath();

    InputStream * newCopyOfStream();

    static uint16_t const MAX_LINES = 0xFFFF;

private:
    static inline bool isDigit_(char c){ return c >= '0' && c <= '9'; }
    static inline bool isAlpha_(char c){
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
                c == '_';
    }

    bool isAtEnd_();
    char peek_();
    char nextChar_();
    void incrementLine_();
    void skipWhitespace_();
    bool matchNext_(char expected);
    
    Token makeToken_(Token::Type type);
    Token makeErrorToken_(const char* message);
    Token makeStringToken_();
    Token makeNumberToken_();
    Token makeIdentifierToken_();
    Token::Type identifierType_();
    Token::Type checkKeyword_(int offset, int len, char const * rest, Token::Type type);

    // Token string buffer:
    static const char MAX_TOKEN_LEN = 64;
    char tokenStr_[MAX_TOKEN_LEN];
    uint16_t tokenStrLen_;

    Mem * mem_;
    InputStream * stream_;
    uint16_t line_, col_;
};
