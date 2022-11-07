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
    int16_t depth;
    uint8_t isDefined;
    uint8_t isConst;

    // Values used by Environment::resolveLocal()
    static int const NOT_INITIALISED = -1;
    static int const NOT_FOUND = -2;
};



// Note: lox calls this a Compiler:
struct Environment {
    static int const MAX_LOCALS = 255;

    Local locals[MAX_LOCALS];
    uint8_t localCount = 0;
    uint16_t scopeDepth = 0;

    /**
     * track a local variables position in the stack
     * @return false if too many locals
     */
    bool addLocal(Token & name, bool isConst);

    /**
     * lookup a local variables position in the stack
     * @return positional index or NOT_FOUND or NOT_INITIALISED
     */
    int resolveLocal(Token & name, bool & isConst);
    
    /**
     * mark latest local as initialised
     */
    void defineLocal();
    
    /**
     * release all locals which are no longer in scope
     * @return number of locals freed
     */
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
    void consume_(Token::Type type, const char* fmt, ...);
    bool match_(Token::Type type);
    Chunk * getCurrentChunk_();
    ParseRule const * getRule_(Token::Type type);

    // parsing different types of things:
    void expression_();
    bool declaration_(bool isExpression);  // returns wasExpression
    bool statement_(bool isExpression);    // returns wasExpression
    void ifStatement_();
    void ifExpression_();
    void if_(bool isExpression);
    void whileStatement_();
    void synchronise_();
    void beginScope_();
    void block_(bool isExpression);
    void expressionBlock_() { block_(true); }
    void nestedBlock_(bool isExpression);
    void endScope_();
    void parse_(Precedence precedence);  // parse expressions with >= precendence
    void type_();
    void print_();
    void and_();
    void or_();
    void number_();
    void string_();
    void variable_(bool canAssign);
    void unary_();
    void binary_();
    void grouping_();  // parentheses in expressions

    // declaring variables:
    void varDeclaration_(bool isConst);
    uint8_t parseVariable_(const char * errorMsg, bool isConst);
    void declareVariable_(bool isConst);
    void defineVariable_(uint8_t global, bool isConst);

    // references to variables:
    void getSetVariable_(Token & token, bool canAssign);

    // bytecode helpers:
    void emitByte_(uint8_t byte);
    void emitByteAtLine_(uint8_t byte, uint16_t line);
    inline void emitBytes_(uint8_t b1, uint8_t b2){ emitByte_(b1); emitByte_(b2); }
    void endCompilation_();
    void emitTrue_();
    void emitFalse_();
    void emitNil_();
    void emitReturn_();
    void emitBoolType_();
    void emitFloatType_();
    void emitObjectType_();
    void emitStringType_();
    void emitTypeIdType_();
    void emitLiteral_(Value value);
    uint8_t makeLiteral_(Value value);
    uint8_t makeIdentifierLiteral_(Token & name);
    int emitJump_(uint8_t instr);
    void setJumpDestination_(int offset);
    void emitLoop_(int loopStart);

    // error production:
    void errorAtCurrent_(const char* fmt, ...);
    void errorAtPrevious_(const char* fmt, ...);
    void errorAt_(Token* token, const char* fmt, ...);
    void errorAtVargs_(Token* token, const char* message, va_list args);

    Vm * vm_;
    Scanner scanner_;
    Chunk * compilingChunk_;
    Environment * currentEnv_;
    Token currentToken_;
    Token previousToken_;
    bool hadError_;
    bool panicMode_;
};
