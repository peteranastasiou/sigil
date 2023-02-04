#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "inputstream/inputstream.hpp"

#include <functional>

class Mem;
class Compiler;

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
  CALL,        // . () []
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

    // Used by Environment when searching for locals:
    static int const NOT_FOUND = -1;
};

/**
 * An "upvalue" is a local which is captured by a function as it becomes a closure
 */
struct Upvalue {
    uint8_t index;  // referenced local's position on the stack
    bool isConst;   // constant or variable
    bool isLocal;   // true: is a local variable, false: is another upvalue
};

/**
 * Environment tracks the local variables and upvalues associated 
 * with each function as it is compiled
 */
struct Environment {
    static int const MAX_LOCALS = 255;
    static int const MAX_UPVALUES = 255;

    enum Type {
        FUNCTION,  // within a function
        SCRIPT     // top-level code
    } type;

    Environment * enclosing;  // the parent environment
    ObjFunction * function = nullptr;
    Upvalue upvalues[MAX_UPVALUES];
    Local locals[MAX_LOCALS];
    uint8_t localCount;
    uint16_t scopeDepth;

    Environment(Mem * mem, ObjString * name, Type t);

    /**
     * track a local variables position in the stack
     * @return false if too many locals
     */
    bool addLocal(Token & name, bool isConst);

    /**
     * lookup a local variables position in the stack
     * Searches innermost to outermost scope within the environment 
     * for a matching name (to support shadowing)
     * @return positional index or NOT_FOUND or NOT_INITIALISED
     */
    int resolveLocal(Compiler * c, Token & name, bool & isConst);

    /**
     * lookup a local in surrounding environments
     * The local is by definition an "upvalue" for this environment
     * @return positional index or NOT_FOUND
     */
    int resolveUpvalue(Compiler * c, Token & name, bool & isConst);

    /**
     * Add an upvalue to the current environment's function
     */
    int addUpvalue(Compiler * c, uint8_t index, bool isConst, bool isLocal);

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
    Compiler(Mem * mem);

    ~Compiler();

    /**
     * @param stream [input]
     * @param chunk [output]
    */
    ObjFunction * compile(InputStream * stream);

private:
    // parser helpers:
    void advance_();
    bool check_(Token::Type type);
    void consume_(Token::Type type, const char* fmt, ...);
    bool match_(Token::Type type);
    Chunk * getCurrentChunk_();
    ParseRule const * getRule_(Token::Type type);

    // parsing different types of things:
    void expression_();
    bool declaration_(bool canBeExpression);  // returns isExpression
    bool statement_(bool canBeExpression);    // returns isExpression
    void ifStatement_();
    void ifExpression_();
    bool if_(bool canBeExpression);           // returns isExpression
    void whileStatement_();
    void synchronise_();
    void beginScope_();
    bool block_(bool canBeExpression);        // returns isExpression
    void expressionBlock_();
    bool nestedBlock_(bool canBeExpression);  // returns isExpression
    void endScope_();
    void parse_(Precedence precedence);  // parse expressions with >= precendence
    void call_();
    void list_();
    void type_();
    void print_();
    void echo_();
    void index_();
    void and_();
    void or_();
    void number_();
    void string_();
    void unary_();
    void binary_();
    void grouping_();  // parentheses in expressions

    // parsing functions:
    void funcDeclaration_();
    void funcAnonymous_();
    void function_(ObjString * name, Environment::Type type);

    // parsing variables:
    void varDeclaration_(bool isConst);
    uint8_t parseVariable_(const char * errorMsg, bool isConst, bool isLocal);
    void declareLocal_(bool isConst);
    void defineVariable_(uint8_t global, bool isConst, bool isLocal);

    // references to variables:
    void variable_(bool canAssign);
    void getSetVariable_(Token & token, bool canAssign);

    // bytecode helpers:
    void emitByte_(uint8_t byte);
    void emitByteAtLine_(uint8_t byte, uint16_t line);
    inline void emitBytes_(uint8_t b1, uint8_t b2){ emitByte_(b1); emitByte_(b2); }
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

    // Environment:
    void initEnvironment_(Environment & env);
    ObjFunction * endEnvironment_();

    // error production:
    void fatalError_(const char* fmt, ...);
    void errorAtCurrent_(const char* fmt, ...);
    void errorAtPrevious_(const char* fmt, ...);
    void errorAt_(Token* token, const char* fmt, ...);
    void errorAtVargs_(Token* token, const char* message, va_list args);

    Mem * mem_;
    Scanner scanner_;
    Environment * currentEnv_;
    Token currentToken_;
    Token previousToken_;
    bool hadError_;
    bool hadFatalError_;
    bool panicMode_;

    friend class Environment; // environment needs to call error functions!
};
