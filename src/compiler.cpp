
#include "compiler.hpp"
#include "debug.hpp"
#include "vm.hpp"

#include <stdio.h>
#include <stdlib.h>

bool Environment::addLocal(Token & name) {
    if( localCount == MAX_LOCALS ){
        return false;
    }

    Local * local = &locals[localCount++];
    local->name = name;
    local->depth = scopeDepth;
    local->init = false;
    return true;
}

void Environment::initialiseLocal() {
    // Now we can replace the NOT_INITIALISED value with the actual depth:
    locals[localCount-1].init = true;
}

int Environment::resolveLocal(Token & name) {
    // search for a local by name in the environment
    // NOTE: searching from higher depth to lower, to support shadowing correctly
    for( int i = localCount-1; i >= 0; i-- ){
        Local * local = &locals[i];
        if( name.equals(local->name) ){
            // Handle special case where it hasn't been initialised before reference
            // e.g. var a = a;
            if( !local->init ){
                return Local::NOT_INITIALISED;
            }

            // The local index happens to also be its position on the stack at runtime:
            return i;
        }
    }
    return Local::NOT_FOUND;
}

uint8_t Environment::freeLocals() {
    uint8_t nFreed = 0;
    // remove all locals which have fallen out of scope:
    while( localCount > 0 && locals[localCount-1].depth > scopeDepth ){
        localCount--;
        nFreed++;
    }
    return nFreed;
}

Compiler::Compiler(Vm * vm) : vm_(vm) {
}

Compiler::~Compiler() {
}

bool Compiler::compile(char const * source, Chunk & chunk) {
    scanner_.init(source);
    Environment env;
    currentEnv_ = &env;
    compilingChunk_ = &chunk;

    hadError_ = false;
    panicMode_ = false;

    advance_();  // get the first token
    
    // compile declarations until we hit the end
    while( !match_(Token::END) ){
        declaration_();
    }

    endCompilation_();
    return !hadError_;
}

void Compiler::advance_() {
    // record last token
    previousToken_ = currentToken_;
    // spin until we get a valid token (or END):
    for(;;) {
        currentToken_ = scanner_.scanToken();
        if( currentToken_.line == Scanner::MAX_LINES ){
            fprintf(stderr, "Too many lines");
            exit(1); // TODO proper error handling
        }

        if( currentToken_.type == Token::ERROR ){
            // report error then ignore and continue
            errorAtCurrent_(currentToken_.start);
        }else{
            // valid token
            return;
        }
    }
}

void Compiler::consume_(Token::Type type, const char* message) {
    // Asserts that the current token is the type specified
    if( currentToken_.type == type ){
        // only advance if token is correct
        advance_();
        return;
    }
    errorAtCurrent_(message);
}

bool Compiler::match_(Token::Type type) {
    // Like consume_ but return bool instead of throwing error
    if( currentToken_.type == type ){
        // only advance if token is correct
        advance_();
        return true;
    }
    return false;
}

Chunk * Compiler::getCurrentChunk_() {
    return compilingChunk_;
}

void Compiler::emitByte_(uint8_t byte) {
    emitByteAtLine_(byte, previousToken_.line);
}

void Compiler::emitByteAtLine_(uint8_t byte, uint16_t line) {
    getCurrentChunk_()->write(byte, line);
}

void Compiler::endCompilation_() {
    emitReturn_();
}

void Compiler::emitReturn_() {
    emitByte_(OpCode::RETURN);
}

void Compiler::emitTrue_() {
    emitByte_(OpCode::TRUE);
}

void Compiler::emitFalse_() {
    emitByte_(OpCode::FALSE);
}

void Compiler::emitNil_() {
    emitByte_(OpCode::NIL);
}

void Compiler::emitLiteral_(Value value) {
    emitBytes_(OpCode::LITERAL, makeLiteral_(value));
}

uint8_t Compiler::makeLiteral_(Value value) {
    uint8_t literal = getCurrentChunk_()->addLiteral(value);
    if( literal == Chunk::MAX_LITERALS ){
        errorAtPrevious_("Too many literals in one chunk.");
        return 0;
    }
    return literal;
}

void Compiler::expression_() {
    parse_(Precedence::ASSIGNMENT);
}

void Compiler::declaration_() {
    if( match_(Token::VAR) ){
        varDeclaration_();
    }else{
        statement_();
    }

    // End of a statement is a good place to re-sync the parser if its panicking
    if( panicMode_ ) synchronise_();
}

void Compiler::varDeclaration_() {
    // Load the variable name, getting the literals index (if global) or 0 (if local):
    uint8_t globalName = parseVariable_("Expected variable name.");

    // assigned an initial value?
    if( match_(Token::EQUAL) ){
        expression_();
    }else{
        emitByte_(OpCode::NIL); // default value is nil
    }
    consume_(Token::SEMICOLON, "Expected ';' after var declaration.");
    defineVariable_(globalName);
}

void Compiler::defineVariable_(uint8_t global) {
    if( currentEnv_->scopeDepth > 0 ){
        // record that the local var is now initialised
        currentEnv_->initialiseLocal();
        // Don't need to do produce any bytecode here
        // At runtime the local's value will already on the stack
        return;
    }

    // global variables get defined at runtime:
    emitBytes_(OpCode::DEFINE_GLOBAL, global);
}

void Compiler::and_() {
    // left hand side has already been compiled, 
    // if its falsy, we want to jump over the right hand side (short circuiting)
    int jumpOverRhs = emitJump_(OpCode::JUMP_IF_FALSE);
    emitByte_(OpCode::POP);   // don't need the lhs anymore, if we got here - its true!
    parse_(Precedence::AND);  // the rhs value
    setJumpDestination_(jumpOverRhs);
}

void Compiler::or_() {
    // left hand side has already been compiled.
    // if its truthy, jump over the right hand side (short circuiting)
    int jumpOverRhs = emitJump_(OpCode::JUMP_IF_TRUE);
    emitByte_(OpCode::POP);  // don't need the lhs anymore
    parse_(Precedence::OR);  // the rhs value
    setJumpDestination_(jumpOverRhs);
}

void Compiler::statement_() {
    if( match_(Token::PRINT) ){
        // print statement takes a single value:
        expression_();
        consume_(Token::SEMICOLON, "Expected ';' after statement.");
        emitByte_(OpCode::PRINT);

    }else if( match_(Token::IF) ){
        ifStatement_();

    }else if( match_(Token::WHILE) ){
        whileStatement_();

    }else if( match_(Token::LEFT_BRACE) ){
        // recurse into a nested scope:
        beginScope_();
        block_();
        endScope_();

    }else{
        // expression statement:
        expression_();
        consume_(Token::SEMICOLON, "Expected ';' after statement.");
        emitByte_(OpCode::POP); // discard the result
    }
}

void Compiler::ifStatement_() {
    // the condition part surrounded by parens:
    consume_(Token::LEFT_PAREN, "Expected '(' after 'if'.");
    expression_();
    consume_(Token::RIGHT_PAREN, "Expected ')' after condition.");
    // the jump instruction (we don't know how far yet)
    int jumpOverThen = emitJump_(OpCode::JUMP_IF_FALSE_POP);
    // the `then` block:
    statement_();
    // optional `else` block:
    if( match_(Token::ELSE) ){
        // jump over the `else` block:
        int jumpOverElse = emitJump_(OpCode::JUMP);
        // this is where `else` starts:
        setJumpDestination_(jumpOverThen);
        // the `else` block:
        statement_();
        setJumpDestination_(jumpOverElse);
    }else{
        // no else block, simply jump to here if false:
        setJumpDestination_(jumpOverThen);
    }
}

void Compiler::whileStatement_() {
    consume_(Token::LEFT_PAREN, "Expected '(' after 'while'.");
    // check the condition (this is where we loop):
    int loopStart = getCurrentChunk_()->count();
    expression_();
    consume_(Token::RIGHT_PAREN, "Expected ')' after condition.");
    // jump over the body if falsy
    int jumpToEnd = emitJump_(OpCode::JUMP_IF_FALSE_POP);
    statement_();
    // loop back up
    emitLoop_(loopStart);
    // escape the loop to here:
    setJumpDestination_(jumpToEnd);
}

void Compiler::synchronise_() {
    // try and find a boundary which seems like a good sync point
    panicMode_ = false;
    while( currentToken_.type != Token::END ){
        // stop if the previous token looks like the end of a declaration/statement:
        if( previousToken_.type == Token::SEMICOLON ) return;

        // the following tokens look like the start of a new declaration/statement:
        switch( currentToken_.type ){
            case Token::FN:
            case Token::VAR:
            case Token::FOR:
            case Token::IF:
            case Token::WHILE:
            case Token::PRINT:
            case Token::RETURN:
                return;

            default: break;  // keep spinning
        }
        advance_();
    }
}

void Compiler::beginScope_() {
    currentEnv_->scopeDepth++;
}

void Compiler::endScope_() {
    currentEnv_->scopeDepth--;

    // At the end of a scope, remove all local variables from the value stack
    uint8_t n = currentEnv_->freeLocals();
    emitBytes_(OpCode::POP_N, n);
}

void Compiler::block_() {
    // parse declarations (and statements) until hit the closing brace
    while( currentToken_.type != Token::RIGHT_BRACE && 
           currentToken_.type != Token::END ){
        declaration_();
    }
    consume_(Token::RIGHT_BRACE, "Expected '}' after block.");
}

void Compiler::parse_(Precedence precedence) {
    // Next token
    advance_();

    // Perform prefix rule of the token first:
    auto prefixRule = getRule_(previousToken_.type)->prefix;
    if( prefixRule == NULL ){
        errorAtPrevious_("Expected expression");
        return;
    }
    // Check whether assignment is possible and pass down to the rule (if it cares)
    bool canAssign = precedence <= Precedence::ASSIGNMENT;
    prefixRule(canAssign);

    // Perforce infix rules on tokens from left to right:
    for( ;; ){
        ParseRule const * rule = getRule_(currentToken_.type);
        if( rule->precedence < precedence ){
            // Stop: the new token has lower precedence so is not part of the current operand
            break;
        }
        // Consume and then compile the operator:
        advance_();
        rule->infix(canAssign);  // Can't be NULL as Precedence > NONE (refer getRule_ table)
    }
    // handle a case where assignment is badly placed, otherwise this isn't handled!
    if( canAssign && match_(Token::EQUAL) ){
        errorAtPrevious_("Invalid assignment target.");
    }
}

uint8_t Compiler::parseVariable_(const char * errorMsg) {
    consume_( Token::IDENTIFIER, errorMsg );

    declareVariable_();

    // if its a local being declared, we don't need to store its name as a literal: 
    if( currentEnv_->scopeDepth > 0 ) return 0;

    // globals get their names stored as a literal:
    return makeIdentifierLiteral_(previousToken_);
}

uint8_t Compiler::makeIdentifierLiteral_(Token & name) {
    return makeLiteral_(Value::object(
        ObjString::newString(vm_, name.start, name.length)
    ));
}

void Compiler::declareVariable_() {
    // Don't need to track globals (resolved at runtime):
    if( currentEnv_->scopeDepth == 0 ) return;

    // the name of the new local variable:
    Token * name = &previousToken_;

    // ensure the variable is not already declared in this scope!
    for( int i = currentEnv_->localCount-1; i>=0; i-- ){
        Local * local = &currentEnv_->locals[i];
        if( local->depth < currentEnv_->scopeDepth ){
            break;  // left the scope - stop searching
        }
        if( name->equals(local->name) ){
            errorAtPrevious_("Already a variable with this name in this scope.");
        }
    }

    // New local variable to track:
    if( !currentEnv_->addLocal(*name) ){
        errorAtPrevious_("Too many local variables in function.");
    }
}

int Compiler::emitJump_(uint8_t instr) {
    emitByte_(instr);
    // placeholder value:
    emitByte_(0xFF);
    emitByte_(0xFF);
    // location of placeholder
    return getCurrentChunk_()->count() - 2;
}

void Compiler::setJumpDestination_(int offset) {
    Chunk * chunk = getCurrentChunk_();

    // how far to jump:
    int jumpLen = chunk->count() - offset - 2;
    if( jumpLen > UINT16_MAX ){
        errorAtPrevious_("Too much code to jump over.");
    }
    // set value:
    chunk->getCode()[offset] = (uint8_t)(jumpLen >> 8);
    chunk->getCode()[offset+1] = (uint8_t)(jumpLen & 0xFF);
}


void Compiler::emitLoop_(int loopStart) {
    emitByte_(OpCode::LOOP);
    int offset = getCurrentChunk_()->count() - loopStart + 2;
    if( offset > UINT16_MAX ) errorAtPrevious_("Loop body is too large.");
    emitByte_((uint8_t)((offset >> 8) & 0xff));
    emitByte_((uint8_t)(offset & 0xff));
}

void Compiler::grouping_() {
    // The opening '( is already consumed, expect an expression next:
    expression_();

    // consume the closing brace:
    consume_(Token::RIGHT_PAREN, "Expected ')' after expression");
}

void Compiler::unary_() {
    Token::Type operatorType = previousToken_.type;
    uint16_t line = previousToken_.line;

    // Compile the operand evaluation first:
    parse_(Precedence::UNARY);

    // Result of the operand gets negated:
    switch( operatorType ){
        case Token::BANG:  emitByteAtLine_(OpCode::NOT, line); break;
        case Token::MINUS: emitByteAtLine_(OpCode::NEGATE, line); break;
        default: break;
    }
}

void Compiler::binary_() {
    // infix operator just got consumed, next token is the start of the second operand
    // the first operand is already compiled and will end up on the stack first
    Token::Type operatorType = previousToken_.type;
    ParseRule const * rule = getRule_(operatorType);

    // parse the second operand, and stop when the precendence is equal or lower
    // stopping when precedence is equal causes math to be left associative: 1+2+3 = (1+2)+3
    parse_((Precedence)((int)rule->precedence + 1));

    // now both operand values will end up on the stack. combine them:
    switch( operatorType ){
        case Token::BANG_EQUAL:    emitByte_(OpCode::NOT_EQUAL); break;
        case Token::EQUAL_EQUAL:   emitByte_(OpCode::EQUAL); break;
        case Token::GREATER:       emitByte_(OpCode::GREATER); break;
        case Token::GREATER_EQUAL: emitByte_(OpCode::GREATER_EQUAL); break;
        case Token::LESS:          emitByte_(OpCode::LESS); break;
        case Token::LESS_EQUAL:    emitByte_(OpCode::LESS_EQUAL); break;
        case Token::PLUS:          emitByte_(OpCode::ADD); break;
        case Token::MINUS:         emitByte_(OpCode::SUBTRACT); break;
        case Token::STAR:          emitByte_(OpCode::MULTIPLY); break;
        case Token::SLASH:         emitByte_(OpCode::DIVIDE); break;
        default: break;
    }
}

void Compiler::number_() {
    // shouldn't fail as we already validated the token as a number:
    double n = strtod(previousToken_.start, nullptr);
    emitLiteral_(Value::number(n));
}

void Compiler::string_() {
    ObjString * str = ObjString::newString(vm_, previousToken_.start+1, previousToken_.length-2);
    emitLiteral_(Value::object(str));
}

void Compiler::variable_(bool canAssign) {
    namedVariable_(previousToken_, canAssign);
}

void Compiler::namedVariable_(Token & token, bool canAssign) {
    uint8_t getOp, setOp, arg; // opcodes for getting and setting the variable, and their argument

    // first, try to look up
    int res = currentEnv_->resolveLocal(token);
    if( res == Local::NOT_INITIALISED ){
        errorAtPrevious_("Local variable referenced before definition.");
    } else if( res == Local::NOT_FOUND ){
        // its a global variable
        getOp = OpCode::GET_GLOBAL;
        setOp = OpCode::SET_GLOBAL;
        arg = makeIdentifierLiteral_(token);  // arg is the literal index of the globals name
    }else{
        // its a local variable
        getOp = OpCode::GET_LOCAL;
        setOp = OpCode::SET_LOCAL;
        arg = (uint8_t)res;  // arg is the stack position of the local var
    }

    // identify whether we are setting or getting a variable:
    if( canAssign && match_(Token::EQUAL) ){
        // setting the variable:
        expression_();  // the value to set
        emitBytes_(setOp, (uint8_t)arg);
    }else{
        // getting the variable:
        emitBytes_(getOp, (uint8_t)arg);
    }
}

// Macros to define lambdas to call each function with or without parameter `canAssign`
#define ASSIGNMENT_RULE(fn) [this](bool canAssign){ this->fn(canAssign); }
#define RULE(fn) [this](bool canAssign){ (void) canAssign; this->fn(); }

ParseRule const * Compiler::getRule_(Token::Type type) {
    static const ParseRule rules[] = {
        // token type             prefix func      infix func     infix precedence
        [Token::LEFT_PAREN]    = {RULE(grouping_), NULL,          Precedence::NONE},
        [Token::RIGHT_PAREN]   = {NULL,            NULL,          Precedence::NONE},
        [Token::LEFT_BRACE]    = {NULL,            NULL,          Precedence::NONE}, 
        [Token::RIGHT_BRACE]   = {NULL,            NULL,          Precedence::NONE},
        [Token::COMMA]         = {NULL,            NULL,          Precedence::NONE},
        [Token::MINUS]         = {RULE(unary_),    RULE(binary_), Precedence::TERM},
        [Token::PLUS]          = {NULL,            RULE(binary_), Precedence::TERM},
        [Token::SEMICOLON]     = {NULL,            NULL,          Precedence::NONE},
        [Token::SLASH]         = {NULL,            RULE(binary_), Precedence::FACTOR},
        [Token::STAR]          = {NULL,            RULE(binary_), Precedence::FACTOR},
        [Token::BANG]          = {RULE(unary_),    NULL,          Precedence::NONE},
        [Token::BANG_EQUAL]    = {NULL,            RULE(binary_), Precedence::EQUALITY},
        [Token::EQUAL]         = {NULL,            NULL,          Precedence::NONE},
        [Token::EQUAL_EQUAL]   = {NULL,            RULE(binary_), Precedence::EQUALITY},
        [Token::GREATER]       = {NULL,            RULE(binary_), Precedence::COMPARISON},
        [Token::GREATER_EQUAL] = {NULL,            RULE(binary_), Precedence::COMPARISON},
        [Token::LESS]          = {NULL,            RULE(binary_), Precedence::COMPARISON},
        [Token::LESS_EQUAL]    = {NULL,            RULE(binary_), Precedence::COMPARISON},
        [Token::IDENTIFIER]    = {ASSIGNMENT_RULE(variable_), NULL,  Precedence::NONE},
        [Token::STRING]        = {RULE(string_),   NULL,          Precedence::NONE},
        [Token::NUMBER]        = {RULE(number_),   NULL,          Precedence::NONE},
        [Token::AND]           = {NULL,            RULE(and_),    Precedence::NONE},
        [Token::CONST]         = {NULL,            NULL,          Precedence::NONE},
        [Token::ELSE]          = {NULL,            NULL,          Precedence::NONE},
        [Token::FALSE]         = {RULE(emitFalse_),NULL,          Precedence::NONE},
        [Token::FOR]           = {NULL,            NULL,          Precedence::NONE},
        [Token::FN]            = {NULL,            NULL,          Precedence::NONE},
        [Token::IF]            = {NULL,            NULL,          Precedence::NONE},
        [Token::NIL]           = {RULE(emitNil_),  NULL,          Precedence::NONE},
        [Token::OR]            = {NULL,            RULE(or_),     Precedence::NONE},
        [Token::PRINT]         = {NULL,            NULL,          Precedence::NONE},
        [Token::RETURN]        = {NULL,            NULL,          Precedence::NONE},
        [Token::TRUE]          = {RULE(emitTrue_), NULL,          Precedence::NONE},
        [Token::VAR]           = {NULL,            NULL,          Precedence::NONE},
        [Token::WHILE]         = {NULL,            NULL,          Precedence::NONE},
        [Token::ERROR]         = {NULL,            NULL,          Precedence::NONE},
        [Token::END]           = {NULL,            NULL,          Precedence::NONE},
    };
    return &rules[type];
}
#undef RULE

void Compiler::errorAtCurrent_(const char* message) {
    errorAt_(&currentToken_, message);
}

void Compiler::errorAtPrevious_(const char* message) {
  errorAt_(&previousToken_, message);
}

void Compiler::errorAt_(Token* token, const char* message) {
    if( panicMode_ ) return;  // suppress errors after the first
    panicMode_ = true;

    fprintf(stderr, "%d: Error", token->line);

    if (token->type == Token::END) {
        fprintf(stderr, " at end");
    } else if (token->type == Token::ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    hadError_ = true;
}

