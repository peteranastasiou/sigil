#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include <functional>

class Vm;

// Precedence order from lowest to highest:
enum class Precedence {
  NONE,
  ASSIGNMENT,  // =
  OR,          // or
  AND,         // and
  EQUALITY,    // == !=
  COMPARISON,  // < > <= >=
  TERM,        // + -
  FACTOR,      // * /
  UNARY,       // ! -
  CALL,        // . ()
  PRIMARY
};

// Parse rule to define how to parse each token:
struct ParseRule {
    std::function<void(bool)> prefix;
    std::function<void(bool)> infix;
    Precedence precedence;
};

struct Local {
    Token name;
    int depth;
};

// Note: lox calls this a Compiler:
struct Environment {
    static int const MAX_LOCALS = 255;

    Local locals[MAX_LOCALS];
    uint8_t localCount = 0;
    uint16_t scopeDepth = 0;

    bool addLocal(Token name);
    uint8_t freeLocals();
};

// Note: lox calls this a Parser:
class Compiler {
public:
    Compiler(Vm * vm);

    ~Compiler();

    /**
     * @param source [input]
     * @param chunk [output]
    */
    bool compile(char const * source, Chunk & chunk);

private:
    // parser helpers:
    void advance_();
    void consume_(Token::Type type, const char* message);
    bool match_(Token::Type type);
    Chunk * currentChunk_();
    ParseRule const * getRule_(Token::Type type);

    // parsing different types of things:
    void expression_();
    void declaration_();
    void varDeclaration_();
    void defineVariable_(uint8_t global);
    void statement_();
    void synchronise_();
    void beginScope_();
    void block_();
    void endScope_();
    void parse_(Precedence precedence);  // parse expressions with >= precendence
    uint8_t parseVariable_(const char * errorMsg);
    void number_();
    void string_();
    void variable_(bool canAssign);
    void namedVariable_(Token token, bool canAssign);
    void unary_();
    void binary_();
    void grouping_();  // parentheses in expressions

    // bytecode helpers:
    void emitByte_(uint8_t byte);
    void emitByteAtLine_(uint8_t byte, uint16_t line);
    inline void emitBytes_(uint8_t b1, uint8_t b2){ emitByte_(b1); emitByte_(b2); }
    void endCompilation_();
    void emitTrue_();
    void emitFalse_();
    void emitNil_();
    void emitReturn_();
    void emitLiteral_(Value value);
    uint8_t makeLiteral_(Value value);
    uint8_t makeIdentifierLiteral_(Token & name);
    void declareVariable_();
    void addLocal_();

    // error production:
    void errorAtCurrent_(const char* message);
    void errorAtPrevious_(const char* message);
    void errorAt_(Token* token, const char* message);

    Vm * vm_;
    Scanner scanner_;
    Chunk * compilingChunk_;
    Environment * currentEnv_;
    Token currentToken_;
    Token previousToken_;
    bool hadError_;
    bool panicMode_;
};
