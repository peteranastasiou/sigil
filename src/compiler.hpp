#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include "inputstream/inputstream.hpp"

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

struct Local {
    ObjString * name;
    int16_t depth;
    bool isDefined;
    bool isConst;
    bool isCaptured;  // whether it is referenced by another function

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
    bool addLocal(ObjString * name, bool isConst);

    /**
     * lookup a local variables position in the stack
     * Searches innermost to outermost scope within the environment 
     * for a matching name (to support shadowing)
     * @return positional index or NOT_FOUND or NOT_INITIALISED
     */
    int resolveLocal(Compiler * c, ObjString * name, bool & isConst);

    /**
     * lookup a local in surrounding environments
     * The local is by definition an "upvalue" for this environment
     * @return positional index or NOT_FOUND
     */
    int resolveUpvalue(Compiler * c, ObjString * name, bool & isConst);

    /**
     * Add an upvalue to the current environment's function
     */
    int addUpvalue(Compiler * c, uint8_t index, bool isConst, bool isLocal);

    /**
     * mark latest local as initialised
     * @return the index of the latest local
     */
    uint8_t defineLocal();

    /**
     * Enter another level of scope depth
     */
    void beginScope();

    /**
     * Exit a level of scope depth
     * release all locals which are no longer in scope
     * enclose all locals which are captured as upvalues
     */
    void endScope(Compiler * c);
};

class Compiler {
public:
    Compiler(Mem * mem);

    ~Compiler();

    /**
     * @param stream [input]
     * @param chunk [output]
    */
    ObjFunction * compile(const char * name, InputStream * stream);

    // Mark root objects to prevent from garbage collection
    void gcMarkRoots();

private:
    // parser helpers:
    void advance_();
    bool check_(Token::Type type);
    bool peekAndCheck_(Token::Type type);
    void consume_(Token::Type type, const char* fmt, ...);
    bool match_(Token::Type type);
    Chunk * getCurrentChunk_();

    // parsing code structures:
    void expression_();
    bool declaration_(bool canBeExpression);  // returns isExpression
    bool statement_(bool canBeExpression);    // returns isExpression
    void ifExpression_();
    bool if_(bool canBeExpression);           // returns isExpression
    void whileStatement_();
    void forExpression_();
    bool for_(bool canBeExpression);          // returns isExpression
    bool forBody_(bool canBeExpression, uint8_t outputLocal);
    bool block_(bool canBeExpression);        // returns isExpression
    void expressionBlock_();
    bool nestedBlock_(bool canBeExpression);  // returns isExpression
    void synchronise_();
    void parse_(Precedence precedence);  // parse expressions with >= precendence

    // parsing operations:
    // all return true if the token was valid for the operation and the operation created
    Precedence getInfixPrecedence_(Token::Type type);
    bool prefixOperation_(Token::Type type, bool canAssign);
    bool infixOperation_(Token::Type type);
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
    void binary_(OpCode opCode);
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
    void getSetVariable_(ObjString * name, bool canAssign);

    // bytecode helpers:
    void emitInstruction_(OpCode instr);
    void emitInstructionArg_(uint8_t arg);
    void emitInstruction_(OpCode instr, uint8_t arg);
    void writeToCodeChunk_(uint8_t byte);
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
    uint8_t makeIdentifierLiteral_(ObjString * name);
    int emitJump_(OpCode instr);
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
    ObjString * name_;
    Scanner scanner_;
    Environment * currentEnv_;
    Token currentToken_;
    Token previousToken_;
    bool hadError_;
    bool hadFatalError_;
    bool panicMode_;

    friend class Environment; // environment needs to call error functions!
};
