
#include "compiler.hpp"
#include "debug.hpp"
#include "mem.hpp"
#include "function.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <vector>


Environment::Environment(Mem * mem, ObjString * name, Type t) {
    type = t;
    localCount = 0;
    scopeDepth = 0;
    function = new ObjFunction(mem, name);

    // Claim first local, reserving space for the "stack pointer"
    Local * local = &locals[0];
    local->name = mem->EMPTY_STRING;
    local->depth = 0;
    local->isDefined = false;
    local->isConst = false;
    local->isCaptured = false;

    // Now we have constructed the string, we can include the first local
    localCount = 1;
}


bool Environment::addLocal(ObjString * name, bool isConst) {
    if( localCount == MAX_LOCALS ){
        return false;
    }

    Local * local = &locals[localCount++];
    local->name = name;
    local->depth = scopeDepth;
    local->isDefined = false;
    local->isConst = isConst;
    local->isCaptured = false;
    return true;
}

uint8_t Environment::defineLocal() {
    // Mark local as having a value now:
    uint8_t latest = (uint8_t)(localCount - 1);
    locals[latest].isDefined = true;
    return latest;
}

int Environment::resolveLocal(Compiler * c, ObjString * name, bool & isConst) {
    // search for a local by name in the environment
    // NOTE: searching from inner (deepest) to outer, to support shadowing correctly
    for( int i = localCount-1; i >= 0; i-- ){
        Local * local = &locals[i];
        if( name == local->name ){
            // Handle special case where it hasn't been initialised before reference
            // e.g. var a = a;
            if( !local->isDefined ){
                c->errorAtPrevious_("Local variable referenced before definition.");
            }

            // Found it:
            isConst = local->isConst;
            // The local index happens to also be its position on the stack at runtime:
            return i;
        }
    }
    return Local::NOT_FOUND;
}

int Environment::resolveUpvalue(Compiler * c, ObjString * name, bool & isConst) {
    // can't check enclosing env if already top-level:
    if( enclosing == nullptr ) return Local::NOT_FOUND;

    // search for local in enclosing environment/function:
    int local = enclosing->resolveLocal(c, name, isConst);
    if( local != Local::NOT_FOUND ){
        // that local is now captured
        enclosing->locals[local].isCaptured = true;
        return addUpvalue(c, (uint8_t)local, isConst, true);
    }

    // search for upvalue in enclosing environment/function:
    int upvalue = enclosing->resolveUpvalue(c, name, isConst);
    if( upvalue != Local::NOT_FOUND ){
        return addUpvalue(c, (uint8_t)upvalue, isConst, false);
    }

    return Local::NOT_FOUND;
}

int Environment::addUpvalue(Compiler * c, uint8_t index, bool isConst, bool isLocal) {
    int n = function->numUpvalues;

    for( int i = 0; i < n; i++ ){
        Upvalue * upvalue = &upvalues[i];
        if( upvalue->index == index && upvalue->isLocal == isLocal ){
            return i; // already got it!
        }
    }

    if( n == MAX_UPVALUES ){
        c->errorAtPrevious_("Too many closure variables in function.");
        return 0;
    }

    upvalues[n] = {.index=index, .isConst=isConst, .isLocal=isLocal};
    return function->numUpvalues++;
}

void Environment::beginScope() {
    scopeDepth++;
}

void Environment::endScope(Compiler * c) {
    scopeDepth--;

    // pop all locals which have fallen out of scope:
    while( localCount > 0 && locals[localCount-1].depth > scopeDepth ){
        if( locals[localCount-1].isCaptured ){
            c->emitInstruction_(OpCode::CLOSE_UPVALUE);
        }else{
            c->emitInstruction_(OpCode::POP);
        }
        localCount--;
    }
}

Compiler::Compiler(Mem * mem) : mem_(mem) {
    name_ = nullptr;
}

Compiler::~Compiler() {
}

ObjFunction * Compiler::compile(const char * name, InputStream * stream) {
    currentEnv_ = nullptr;

    // Capture the name of the script
    name_ = ObjString::newString(mem_, name);

    // Start the scanner
    scanner_.init(mem_, stream);

    Environment env(mem_, name_, Environment::SCRIPT);
    initEnvironment_(env);

    hadError_ = false;
    hadFatalError_ = false;
    panicMode_ = false;

    advance_();  // get the first token
    
    // compile declarations until we hit the end
    while( !match_(Token::END) ){
        declaration_(false);

        if( hadFatalError_ ) break;
    }

    ObjFunction * function = endEnvironment_();
    return hadError_ ? nullptr : function;
}

void Compiler::gcMarkRoots() {
    // Iterate up through nested environments, marking objects as in use
    Environment * env = currentEnv_;
    while( env != nullptr ){
        // Mark function
        if( env->function != nullptr ){
            env->function->gcMark();
        }
        // Mark the names of locals
        for( int i = 0; i < env->localCount; i++ ){
            env->locals[i].name->gcMark();
        }

        env = env->enclosing;
    }

    // Mark the script name, once provided
    if( name_ ) name_->gcMark();

    if( currentToken_.string ) currentToken_.string->gcMark();
    if( previousToken_.string ) previousToken_.string->gcMark();
}

void Compiler::initEnvironment_(Environment & env) {
    env.enclosing = currentEnv_;
    currentEnv_ = &env;
}

ObjFunction * Compiler::endEnvironment_() {
    emitReturn_();
    ObjFunction * fn = currentEnv_->function;
    currentEnv_ = currentEnv_->enclosing;
    return fn;
}

void Compiler::advance_() {
    // record last token
    previousToken_ = currentToken_;
    // spin until we get a valid token (or END):
    for(;;) {
        currentToken_ = scanner_.scanToken();
        if( currentToken_.line == Scanner::MAX_LINES ){
            fatalError_("Too many lines in script");
            // pretend this is the end of the script
            currentToken_.type = Token::END;
            return;
        }

        if( currentToken_.type == Token::ERROR ){
            // report error then ignore and continue
            errorAtCurrent_(currentToken_.string->getCString());
        }else{
            // valid token
            return;
        }
    }
}

bool Compiler::check_(Token::Type type) {
    return currentToken_.type == type;
}

void Compiler::consume_(Token::Type type, const char* fmt, ...) {
    // Asserts that the current token is the type specified
    if( check_(type) ){
        // only advance if token is correct
        advance_();
        return;
    }
    va_list args;
    va_start(args, fmt);
    errorAtVargs_(&currentToken_, fmt, args);
    va_end(args);
}

bool Compiler::match_(Token::Type type) {
    // Like consume_ but return bool instead of throwing error
    if( check_(type) ){
        // only advance if token is correct
        advance_();
        return true;
    }
    return false;
}

Chunk * Compiler::getCurrentChunk_() {
    return &currentEnv_->function->chunk;
}

void Compiler::emitInstruction_(OpCode instr) {
    // Work out the stack impact of the instruction:
    writeToCodeChunk_((uint8_t)instr);
}

void Compiler::emitInstruction_(OpCode instr, uint8_t arg) {
    emitInstruction_(instr);
    writeToCodeChunk_(arg);
}

void Compiler::emitInstructionArg_(uint8_t arg) {
    writeToCodeChunk_(arg);
}

void Compiler::writeToCodeChunk_(uint8_t byte) {
    uint16_t line = previousToken_.line;
    if( !getCurrentChunk_()->write((uint8_t)byte, line) ){
        if( currentEnv_->type == Environment::FUNCTION ){
            fatalError_("Too much code in function.");
        }else{
            fatalError_("Too much code in top level of script.");
        }
    }
}

void Compiler::emitReturn_() {
    emitInstruction_(OpCode::NIL); // implicit return value
    emitInstruction_(OpCode::RETURN);
}

// TODO abstract all use of opcode to emit functions so we can predict stack loc

void Compiler::emitTrue_() {
    emitInstruction_(OpCode::TRUE);
}

void Compiler::emitFalse_() {
    emitInstruction_(OpCode::FALSE);
}

void Compiler::emitNil_() {
    emitInstruction_(OpCode::NIL);
}

void Compiler::emitBoolType_() {
    emitInstruction_(OpCode::TYPE_BOOL);
}

void Compiler::emitFloatType_() {
    emitInstruction_(OpCode::TYPE_FLOAT);
}

void Compiler::emitObjectType_() {
    emitInstruction_(OpCode::TYPE_FUNCTION);
}

void Compiler::emitStringType_() {
    emitInstruction_(OpCode::TYPE_STRING);
}

void Compiler::emitTypeIdType_() {
    emitInstruction_(OpCode::TYPE_TYPEID);
}

void Compiler::emitLiteral_(Value value) {
    emitInstruction_(OpCode::LITERAL, makeLiteral_(value));
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

bool Compiler::declaration_(bool canBeExpression) {
    bool isExpression = false;
    if( match_(Token::VAR) ){
        varDeclaration_(false);

    }else if( match_(Token::CONST) ){
        varDeclaration_(true);

    }else if( match_(Token::FN) ){
        funcDeclaration_();

    }else{
        isExpression = statement_(canBeExpression);
    }

    // End of a statement is a good place to re-sync the parser if it is panicking
    if( panicMode_ && !isExpression ) synchronise_();

    return isExpression;
}

void Compiler::funcDeclaration_() {
    bool isLocal = currentEnv_->scopeDepth > 0;
    bool isConst = true;  // Disallow redefining functions

    // Load the function variable name, getting the literals index (if global) or 0 (if local):
    uint8_t global = parseVariable_("Expected function name.", isConst, isLocal);

    // capture function name for the environment too:
    ObjString * name = previousToken_.string;

    // If its a local, mark it as already defined (allowing for self-referential functions):
    // This is not an issue for globals
    if( isLocal ) currentEnv_->defineLocal();

    // parse arguments and function content
    function_(name, Environment::FUNCTION);

    // assign function literal to variable
    defineVariable_(global, isConst, isLocal);
}

void Compiler::funcAnonymous_() {
    // Parse a function used in an expression: fn(args) { statements }
    function_(mem_->EMPTY_STRING, Environment::FUNCTION);
}

void Compiler::function_(ObjString * name, Environment::Type type) {
    // new environment
    Environment env(mem_, name, type);
    initEnvironment_(env);
    env.beginScope();

    // (i.e. function as expression)
    consume_(Token::LEFT_PAREN, "Expected '(' for function.");
    // if has any parameters:
    if( !check_(Token::RIGHT_PAREN) ){
        do {
            // TODO named params

            // count parameters
            if( ++env.function->numInputs > 255 ){
                errorAtCurrent_("Can't have over 255 parameters.");
            }
            // make a new local at the top of the function's value stack to use as the parameter:
            bool isLocal = true;
            bool isConst = false;
            parseVariable_("Expected parameter name.", isConst, isLocal);
            defineVariable_(0, isConst, isLocal);
        } while( match_(Token::COMMA) );
    }
    consume_(Token::RIGHT_PAREN, "Expected ')' after parameters.");
    consume_(Token::LEFT_BRACE, "Expected '{' before function body.");

    bool isExpression = block_(true);
    if( isExpression ){
        // function ends with an expression (omitted semi-colon)
        // to produce an implicit return:
        emitInstruction_(OpCode::RETURN);
    }

    // Note: no actual need to endScope(), as we are done with the Environment now
    // call it so that we can check the stack emptied correctly:
    env.endScope(this);

    // New function literal:
    ObjFunction * fn = endEnvironment_();
    uint8_t literal = makeLiteral_(Value::function(fn));
    // Note: CLOSURE instruction takes a function literal and wraps it to make a Closure
    emitInstruction_(OpCode::CLOSURE, literal);

    // List all the upvalues (variables enclosed by function):
    for( int i = 0; i < fn->numUpvalues; i++ ){
        // track whether it is a local or already an upvalue which is being uplifted:
        emitInstructionArg_(env.upvalues[i].isLocal ? 1 : 0);
        // stack position of value to lift:
        emitInstructionArg_(env.upvalues[i].index);
    }
}

void Compiler::varDeclaration_(bool isConst) {
    // local and global scoped variables are implemented differently:
    bool isLocal = currentEnv_->scopeDepth > 0;

    // Load the variable name, getting the literals index (if global) or 0 (if local):
    uint8_t global = parseVariable_("Expected variable name.", isConst, isLocal);

    // assigned an initial value?
    if( match_(Token::EQUAL) ){
        expression_();
    }else{
        emitInstruction_(OpCode::NIL); // default value is nil
    }
    consume_(Token::SEMICOLON, "Expected ';' after var declaration.");

    defineVariable_(global, isConst, isLocal);
}

uint8_t Compiler::parseVariable_(const char * errorMsg, bool isConst, bool isLocal) {
    // the name of the variable:
    consume_( Token::IDENTIFIER, errorMsg );

    if(isLocal) {
        // local variables are registered to the stack
        declareLocal_(isConst);
        return 0; // not a global
    } else {
        // globals variables have their names stored as a literal:
        return makeIdentifierLiteral_(previousToken_.string);
    }
}

void Compiler::declareLocal_(bool isConst) {
    // the name of the new local variable:
    ObjString * name = previousToken_.string;

    // ensure the variable is not already declared in this scope!
    for( int i = currentEnv_->localCount-1; i>=0; i-- ){
        Local * local = &currentEnv_->locals[i];
        if( local->depth < currentEnv_->scopeDepth ){
            break;  // left the scope - stop searching
        }
        if( name == local->name ){
            errorAtPrevious_("Already a variable called '%s' in this scope.", 
                             name->getCString());
        }
    }

    // New local variable to track:
    if( !currentEnv_->addLocal(name, isConst) ){
        errorAtPrevious_("Too many local variables in function.");
    }
}

void Compiler::defineVariable_(uint8_t global, bool isConst, bool isLocal) {
    if( isLocal ){
        currentEnv_->defineLocal();
    }else if( isConst ){
        emitInstruction_(OpCode::DEFINE_GLOBAL_CONST, global);
    }else{
        emitInstruction_(OpCode::DEFINE_GLOBAL_VAR, global);
    }
}

void Compiler::and_() {
    // left hand side has already been compiled, 
    // if its falsy, we want to jump over the right hand side (short circuiting)
    int jumpOverRhs = emitJump_(OpCode::JUMP_IF_FALSE);
    emitInstruction_(OpCode::POP);   // don't need the lhs anymore, if we got here - its true!
    parse_(Precedence::AND);  // the rhs value
    setJumpDestination_(jumpOverRhs);
}

void Compiler::or_() {
    // left hand side has already been compiled.
    // if its truthy, jump over the right hand side (short circuiting)
    int jumpOverRhs = emitJump_(OpCode::JUMP_IF_TRUE);
    emitInstruction_(OpCode::POP);  // don't need the lhs anymore
    parse_(Precedence::OR);  // the rhs value
    setJumpDestination_(jumpOverRhs);
}

// Can this return true if canBeExpression is false?
bool Compiler::statement_(bool canBeExpression) {
    if( match_(Token::IF) ){
        return if_(canBeExpression);

    }else if( match_(Token::WHILE) ){
        whileStatement_();
        return false;  // statement only (for now!)

    }else if( match_(Token::FOR) ){
        return for_(canBeExpression);

    }else if( match_(Token::LEFT_BRACE) ){
        // recurse into a nested scope:
        return nestedBlock_(canBeExpression);

    }else if( match_(Token::RETURN) ){
        if( currentEnv_->type == Environment::SCRIPT ){
            errorAtPrevious_("Can't return from top-level.");
        }
        if( check_(Token::SEMICOLON) ){
            emitReturn_();
        }else{
            // the return value(s):
            expression_();
            emitInstruction_(OpCode::RETURN);
        }
    }else if( match_(Token::SEMICOLON) ){
        // Empty statement
        return false;
    }else{
        // expression-statement:
        expression_();
    }
    // what we expect next depends on the context of the expression-statement:
    if( !canBeExpression ){
        // ordinary statement:
        consume_(Token::SEMICOLON, "Expected ';' after statement.");
        emitInstruction_(OpCode::POP); // discard the result
        return false;
    }else if( match_(Token::SEMICOLON) ){
        // statement within an expression block:
        emitInstruction_(OpCode::POP); // discard the result
        return false;
    }else if( check_(Token::RIGHT_BRACE) ){
        // The end of an expression block, leave the value on the stack:
        return true;
    }else{
        errorAtCurrent_("Expected ';' or '}'.");
        return false;
    }
}

void Compiler::ifExpression_() {
    bool isExpression = if_(true);
    if( !isExpression ){
        errorAtPrevious_("Expected if-expression, not if-statement.");
    }
}

bool Compiler::if_(bool canBeExpression) {
    // the condition part:
    expression_();
    // jump over the block to the next part:
    int jumpOver = emitJump_(OpCode::JUMP_IF_FALSE_POP);
    // the block
    consume_(Token::LEFT_BRACE, "Expected '{' after condition.");
    bool isExpression = nestedBlock_(canBeExpression);

    // track all the jumps which go straight to the end
    std::vector<int> jumpsToEnd;

    // optional `elif` blocks:
    while( match_(Token::ELIF) ){
        // protect against fallthrough
        jumpsToEnd.push_back(emitJump_(OpCode::JUMP));
        // jump over the previous if/elif-block to here:
        setJumpDestination_(jumpOver);
        // the condition part:
        expression_();
        // jump over the block to the next part:
        jumpOver = emitJump_(OpCode::JUMP_IF_FALSE_POP);
        // the block
        consume_(Token::LEFT_BRACE, "Expected '{' after 'elif'.");
        if( nestedBlock_(canBeExpression) != isExpression ){
            errorAtPrevious_("Inconsistent if-statement/if-expression.");
        }
    }

    // optional `else` block:
    bool hasElse = match_(Token::ELSE);
    if( hasElse || isExpression ){
        // protect against fallthrough
        jumpsToEnd.push_back(emitJump_(OpCode::JUMP));
        // jump over the previous if/elif-block to here:
        setJumpDestination_(jumpOver);
        if( hasElse ){
            // the block
            consume_(Token::LEFT_BRACE, "Expected '{' after 'else'.");
            if( nestedBlock_(canBeExpression) != isExpression ){
                errorAtPrevious_("Inconsistent if-statement/if-expression.");
            }
        }else{
            // implicit else block for if expressions produces nil
            emitInstruction_(OpCode::NIL);
        }
    }else{
        // no else block, so the last "jumpOver" goes to here:
        setJumpDestination_(jumpOver);
    }
    // link up all the end jumps to here
    for( int jump : jumpsToEnd ){
        setJumpDestination_(jump);
    }

    return isExpression;
}

void Compiler::whileStatement_() {
    // check the condition (this is where we loop):
    int loopStart = getCurrentChunk_()->count();
    expression_();
    // jump over the body if falsy
    int jumpToEnd = emitJump_(OpCode::JUMP_IF_FALSE_POP);
    consume_(Token::LEFT_BRACE, "Expected '{' after condition.");
    nestedBlock_(false);
    // loop back up
    emitLoop_(loopStart);
    // escape the loop to here:
    setJumpDestination_(jumpToEnd);
}

void Compiler::forExpression_() {
    bool isExpression = for_(true);
    if( !isExpression ){
        errorAtPrevious_("Expected for-expression, not for-statement.");
    }
}

bool Compiler::forBody_(bool canBeExpression, uint8_t outputLocal) {
    // Compile the body of the for loop
    bool isExpression = nestedBlock_(canBeExpression);
    if( isExpression ) {
        if( canBeExpression ){
            // Accumulate the result into the output value
            emitInstruction_(OpCode::APPEND_LOCAL, outputLocal);
        }else{
            errorAtPrevious_("Expected a statement not an expression.");
        }
    }
    return isExpression;
}

bool Compiler::for_(bool canBeExpression) {
    // For-expressions need an output list value
    uint8_t outputLocal = 0;
    if( canBeExpression ){
        emitInstruction_(OpCode::MAKE_LIST, 0); // initially empty
        if( !currentEnv_->addLocal(mem_->EMPTY_STRING, true) ){
            errorAtPrevious_("Too many local variables in function.");
        }
        outputLocal = currentEnv_->defineLocal();
        printf("output is at %i\n", outputLocal);
    }

    // Scope for the iterator value
    currentEnv_->beginScope();

    // Next up is the iterator
    bool isConst = true;  // iterator is const for the user, but vm can increment it!
    bool isLocal = true;
    parseVariable_("Expected iterator name.", isConst, isLocal);
    consume_(Token::IN, "Expected 'in' after iterator.");

    // The initial value (or the end value if there is no range separator)
    expression_();
    uint8_t iteratorLocal = currentEnv_->defineLocal();
    printf("iterator is at %i\n", iteratorLocal);

    // Ranges are denoted by : or := (indicating exclusive and inclusive of end value)
    bool inclusiveRange = check_(Token::COLON_EQUAL);
    if( match_(Token::COLON) || match_(Token::COLON_EQUAL) ){
        // Evaluate the end value.
        expression_();

        consume_(Token::LEFT_BRACE, "Expected '{' after range.");

    }else{
        consume_(Token::LEFT_BRACE, "Expected ':', ':=' or '{' after value");

        // This means we have an implicit start value
        // We need to rearrange.
        // Put the current iterator value where the end value goes:
        emitInstruction_(OpCode::GET_LOCAL, iteratorLocal);
        // Set the iterator to zero:
        emitInstruction_(OpCode::PUSH_ZERO);
        emitInstruction_(OpCode::SET_LOCAL, iteratorLocal);
        emitInstruction_(OpCode::POP);  // Remove the zero
    }

    // At this point the stack is: (outputList,) iterator, endValue

    // Remember where to loop back to
    int loopStart = getCurrentChunk_()->count();

    bool isExpression = false;

    // To include the final value, do the body before the check
    if( inclusiveRange ){
        isExpression = forBody_(canBeExpression, outputLocal);
    }
    // Compare iterator to end value and put +1, -1 or 0 on the stack
    // This is used both to check when to exit and for the iteration direction
    emitInstruction_(OpCode::COMPARE_ITERATOR);
    int jumpToEnd = emitJump_(OpCode::JUMP_IF_ZERO);

    // At this point, the stack is: (outputList,) iterator, endValue, compareValue

    // To exclude the final value, we do the body after the check
    if( !inclusiveRange ) {
        isExpression = forBody_(canBeExpression, outputLocal);
    }
    // Add the compare value to the iterator:
    emitInstruction_(OpCode::GET_LOCAL, iteratorLocal);
    emitInstruction_(OpCode::ADD);
    emitInstruction_(OpCode::SET_LOCAL, iteratorLocal);
    emitInstruction_(OpCode::POP);  // Clean up the new loop value

    // Loop back 
    emitLoop_(loopStart);

    // This is where we exit
    setJumpDestination_(jumpToEnd);

    emitInstruction_(OpCode::POP);  // Clean up the compare value
    emitInstruction_(OpCode::POP);  // Clean up the end value

    // Pop the iterator
    currentEnv_->endScope(this);
    return isExpression;
}

void Compiler::synchronise_() {
    // don't stop panicking if we have had a fatal error: 
    if( hadFatalError_ ) return;

    // try and find a boundary which seems like a good sync point
    panicMode_ = false;
    while( currentToken_.type != Token::END ){
        // stop if the previous token looks like the end of a declaration/statement:
        if( previousToken_.type == Token::SEMICOLON ) return;

        // the following tokens look like the start of a new declaration/statement:
        switch( currentToken_.type ){
            case Token::CONST:
            case Token::FN:
            case Token::VAR:
            case Token::FOR:
            case Token::IF:
            case Token::WHILE:
            case Token::PRINT:
            case Token::ECHO:
            case Token::RETURN:
                return;

            default: break;  // keep spinning
        }
        advance_();
    }
}

void Compiler::expressionBlock_() {
    bool isExpression = block_(true);
    if( !isExpression ){
        errorAtPrevious_("Expression block must end in an expression.");
    }
}

bool Compiler::block_(bool canBeExpression) {
    // parse declarations (and statements) until hit the closing brace
    bool isExpression = false;
    while( !check_(Token::RIGHT_BRACE) && !check_(Token::END) ){
        if( isExpression ){
            errorAtPrevious_("Expression only allowed at end of block.");
        }
        isExpression = declaration_(canBeExpression);
    }
    consume_(Token::RIGHT_BRACE, "Expected '}' after block.");

    return isExpression;
}

bool Compiler::nestedBlock_(bool canBeExpression) {
    currentEnv_->beginScope();
    bool isExpression = block_(canBeExpression);
    currentEnv_->endScope(this);
    return isExpression;
}

void Compiler::parse_(Precedence precedence) {
    // Next token
    advance_();

    // Perform prefix rule of the token first:
    // Check whether assignment is possible and pass down to the rule (if it cares)
    bool canAssign = precedence <= Precedence::ASSIGNMENT;
    if( !prefixOperation_(previousToken_.type, canAssign) ){
        errorAtPrevious_("Expected expression");
        return;
    }

    // Perform infix rules on tokens from left to right:
    for( ;; ){
        const Token::Type type = currentToken_.type;
        if( getInfixPrecedence_(type) < precedence ) {
            // Stop: the new token has lower precedence so is not part of the current operand
            break;
        }
        // Consume and then compile the operator:
        advance_();
        infixOperation_(type);
    }
    // Handle a case where assignment is badly placed, otherwise this isn't handled!
    if( canAssign && match_(Token::EQUAL) ){
        errorAtPrevious_("Invalid assignment target.");
    }
}

uint8_t Compiler::makeIdentifierLiteral_(ObjString * name) {
    return makeLiteral_(Value::string(name));
}

int Compiler::emitJump_(OpCode instr) {
    emitInstruction_(instr);
    // placeholder value:
    emitInstructionArg_(0xFF);
    emitInstructionArg_(0xFF);
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
    emitInstruction_(OpCode::LOOP);
    int offset = getCurrentChunk_()->count() - loopStart + 2;
    if( offset > UINT16_MAX ) errorAtPrevious_("Loop body is too large.");
    emitInstructionArg_((uint8_t)((offset >> 8) & 0xff));
    emitInstructionArg_((uint8_t)(offset & 0xff));
}

void Compiler::grouping_() {
    // The opening '( is already consumed, expect an expression next:
    expression_();

    // consume the closing brace:
    consume_(Token::RIGHT_PAREN, "Expected ')' after expression");
}

void Compiler::unary_() {
    Token::Type operatorType = previousToken_.type;

    // Compile the operand evaluation first:
    parse_(Precedence::UNARY);

    // Result of the operand gets negated:
    switch( operatorType ){
        case Token::BANG:  emitInstruction_(OpCode::NOT); break;
        case Token::MINUS: emitInstruction_(OpCode::NEGATE); break;
        default: break;
    }
}

void Compiler::binary_(OpCode opCode) {
    // infix operator just got consumed, next token is the start of the second operand
    // the first operand is already compiled and will end up on the stack first
    Token::Type operatorType = previousToken_.type;
    int precedence = (int)getInfixPrecedence_(operatorType);

    // parse the second operand, and stop when the precendence is equal or lower
    // stopping when precedence is equal causes math to be left associative: 1+2+3 = (1+2)+3
    parse_((Precedence)(precedence + 1));

    // now both operand values will end up on the stack. emit the operation to combine theM
    emitInstruction_(opCode);
}

void Compiler::call_() {
    // parse arguments:
    uint8_t argCount = 0;
    if( !check_(Token::RIGHT_PAREN) ) {
        do {
            expression_();
            if( argCount == 255 ) {
                errorAtPrevious_("Can't have more than 255 arguments.");
            }
            argCount ++;
        } while( match_(Token::COMMA) );
    }
    consume_(Token::RIGHT_PAREN, "Expected ')' after arguments.");

    emitInstruction_(OpCode::CALL, argCount);
}

void Compiler::list_() {
    uint8_t numEntries = 0;
    if( !check_(Token::RIGHT_BRACKET) ) {
        do {
            expression_();
            if( numEntries == 255 ){
                errorAtPrevious_("Can't have more than 255 elements in list initialiser.");
            }
            numEntries ++;
        } while( match_(Token::COMMA) );
    }
    consume_(Token::RIGHT_BRACKET, "Expected ']' after list elements.");

    emitInstruction_(OpCode::MAKE_LIST, numEntries);
}

void Compiler::type_() {
    consume_(Token::LEFT_PAREN, "Expected '(' after 'type'.");
    // type built-in takes a single value:
    expression_();
    consume_(Token::RIGHT_PAREN, "Expected ')' after argument.");
    emitInstruction_(OpCode::TYPE);
}

void Compiler::print_() {
    consume_(Token::LEFT_PAREN, "Expected '(' after 'print'.");
    // print built-in takes a single value:
    expression_();
    consume_(Token::RIGHT_PAREN, "Expected ')' after argument.");
    emitInstruction_(OpCode::PRINT);
}

void Compiler::echo_() {
    // print built-in takes a single value:
    expression_();
    emitInstruction_(OpCode::ECHO);
}

void Compiler::index_() {
    expression_();
    consume_(Token::RIGHT_BRACKET, "Expected ']' after index.");
    emitInstruction_(OpCode::INDEX_GET);
}

void Compiler::number_() {
    // shouldn't fail as we already validated the token as a number:
    double n = strtod(previousToken_.string->getCString(), nullptr);
    
    // simplify operation for common values:
    if( n == 0 ){
        emitInstruction_(OpCode::PUSH_ZERO);
    }else if( n == 1 ){
        emitInstruction_(OpCode::PUSH_ONE);
    }else if( n == 2 ){
        emitInstruction_(OpCode::PUSH_TWO);
    }else{
        emitLiteral_(Value::number(n));
    }
}

void Compiler::string_() {
    emitLiteral_(Value::string(previousToken_.string));
}

void Compiler::variable_(bool canAssign) {
    getSetVariable_(previousToken_.string, canAssign);
}

void Compiler::getSetVariable_(ObjString * name, bool canAssign) {
    OpCode getOp, setOp;  // opcodes for getting and setting the variable
    uint8_t arg;          // instruction argument

    // first, try to look up
    bool isConst;
    int res = currentEnv_->resolveLocal(this, name, isConst);
    if( res != Local::NOT_FOUND ){
        // its a local variable
        getOp = OpCode::GET_LOCAL;
        setOp = OpCode::SET_LOCAL;
        arg = (uint8_t)res;  // arg is the stack position of the local var

    }else if((res = currentEnv_->resolveUpvalue(this, name, isConst)) != Local::NOT_FOUND) {
        // its an upvalue
        getOp = OpCode::GET_UPVALUE;
        setOp = OpCode::SET_UPVALUE;
        arg = (uint8_t)res;  // stack position of upvalue

    }else{
        // its a global variable
        isConst = false; // assume not constant - checked at runtime
        getOp = OpCode::GET_GLOBAL;
        setOp = OpCode::SET_GLOBAL;
        arg = makeIdentifierLiteral_(name);  // arg is the literal index of the globals name
    }

    // identify whether we are setting or getting a variable:
    if( canAssign && match_(Token::EQUAL) ){
        if( isConst ){
            errorAtPrevious_("Cannot redefine a const variable.");
        }
        // setting the variable:
        expression_();  // the value to set
        emitInstruction_(setOp, arg);
    }else{
        // getting the variable:
        emitInstruction_(getOp, arg);
    }
}

Precedence Compiler::getInfixPrecedence_(Token::Type type) {
    switch( type ) {
        case Token::LEFT_PAREN:
        case Token::LEFT_BRACKET:
            return Precedence::CALL;

        case Token::STAR:
        case Token::SLASH:
            return Precedence::FACTOR;

        case Token::PLUS:
        case Token::MINUS:
            return Precedence::TERM;

        case Token::GREATER:
        case Token::GREATER_EQUAL:
        case Token::LESS:
        case Token::LESS_EQUAL:
            return Precedence::COMPARISON;

        case Token::BANG_EQUAL:
        case Token::EQUAL_EQUAL:
            return Precedence::EQUALITY;

        case Token::AND:
            return Precedence::AND;

        case Token::OR:
            return Precedence::OR;

        case Token::LEFT_BRACE:
        case Token::EQUAL:
        case Token::BANG:
        case Token::SEMICOLON:
        case Token::RIGHT_PAREN:
        case Token::RIGHT_BRACE:
        case Token::RIGHT_BRACKET:
        case Token::COMMA:
        case Token::IDENTIFIER:
        case Token::STRING:
        case Token::NUMBER:
        case Token::BOOL:
        case Token::CONST:
        case Token::ELIF:
        case Token::ELSE:
        case Token::FALSE:
        case Token::FOR:
        case Token::FN:
        case Token::FLOAT:
        case Token::IF:
        case Token::NIL:
        case Token::OBJECT:
        case Token::PRINT:
        case Token::ECHO:
        case Token::RETURN:
        case Token::STRING_TYPE:
        case Token::TRUE:
        case Token::TYPE:
        case Token::TYPEID:
        case Token::VAR:
        case Token::WHILE:
        case Token::ERROR:
        case Token::END:
        default:
            return Precedence::NONE;
    }
}

bool Compiler::infixOperation_(Token::Type type) {
    switch( type ){
        case Token::LEFT_PAREN:      call_();                           return true;
        case Token::LEFT_BRACKET:    index_();                          return true;

        case Token::STAR:            binary_(OpCode::MULTIPLY);         return true;
        case Token::SLASH:           binary_(OpCode::DIVIDE);           return true;

        case Token::PLUS:            binary_(OpCode::ADD);              return true;
        case Token::MINUS:           binary_(OpCode::SUBTRACT);         return true;

        case Token::GREATER:         binary_(OpCode::GREATER);          return true;
        case Token::GREATER_EQUAL:   binary_(OpCode::GREATER_EQUAL);    return true;
        case Token::LESS:            binary_(OpCode::LESS);             return true;
        case Token::LESS_EQUAL:      binary_(OpCode::LESS_EQUAL);       return true;
        case Token::BANG_EQUAL:      binary_(OpCode::NOT_EQUAL);        return true;
        case Token::EQUAL_EQUAL:     binary_(OpCode::EQUAL);            return true;

        case Token::AND:             and_();                            return true;

        case Token::OR:              or_();                             return true;

        case Token::EQUAL:
        case Token::BANG:
        case Token::SEMICOLON:
        case Token::RIGHT_PAREN:
        case Token::LEFT_BRACE:
        case Token::RIGHT_BRACE:
        case Token::RIGHT_BRACKET:
        case Token::COMMA:
        case Token::DOT:
        case Token::IDENTIFIER:
        case Token::STRING:
        case Token::NUMBER:
        case Token::BOOL:
        case Token::CONST:
        case Token::ELIF:
        case Token::ELSE:
        case Token::FALSE:
        case Token::FOR:
        case Token::FN:
        case Token::FLOAT:
        case Token::IF:
        case Token::NIL:
        case Token::OBJECT:
        case Token::PRINT:
        case Token::ECHO:
        case Token::RETURN:
        case Token::STRING_TYPE:
        case Token::TRUE:
        case Token::TYPE:
        case Token::TYPEID:
        case Token::VAR:
        case Token::WHILE:
        case Token::ERROR:
        case Token::END:
        default:
            return false;
    }
}

bool Compiler::prefixOperation_(Token::Type type, bool canAssign) {
    switch( type ){
        // Control flow
        case Token::LEFT_PAREN:    grouping_(); return true;
        case Token::LEFT_BRACKET:  list_(); return true;
        case Token::LEFT_BRACE:    expressionBlock_(); return true;
        case Token::IF:            ifExpression_(); return true;
        case Token::FOR:           forExpression_(); return true;
        case Token::FN:            funcAnonymous_(); return true;

        // Math
        case Token::MINUS:         unary_(); return true;
        case Token::BANG:          unary_(); return true;

        // Values
        case Token::STRING:        string_();return true;
        case Token::NUMBER:        number_(); return true;
        case Token::TRUE:          emitTrue_(); return true;
        case Token::FALSE:         emitFalse_(); return true;
        case Token::NIL:           emitNil_(); return true;

        // Variables
        case Token::IDENTIFIER:    variable_(canAssign);return true;

        // Types
        case Token::BOOL:          emitBoolType_(); return true;
        case Token::FLOAT:         emitFloatType_(); return true;
        case Token::STRING_TYPE:   emitStringType_(); return true;
        case Token::OBJECT:        emitObjectType_(); return true;
        case Token::TYPEID:        emitTypeIdType_(); return true;

        // Built-in functions
        case Token::PRINT:         print_(); return true;
        case Token::ECHO:          echo_(); return true;
        case Token::TYPE:          type_(); return true;

        // TODO while-expressions
        case Token::WHILE:
            return false;

        // Not handled here
        case Token::RIGHT_PAREN:
        case Token::RIGHT_BRACE:
        case Token::RIGHT_BRACKET:
        case Token::COMMA:
        case Token::PLUS:
        case Token::SEMICOLON:
        case Token::SLASH:
        case Token::STAR:
        case Token::AND:
        case Token::OR:
        case Token::RETURN:
        case Token::CONST:
        case Token::ELIF:
        case Token::ELSE:
        case Token::BANG_EQUAL:
        case Token::EQUAL:
        case Token::EQUAL_EQUAL:
        case Token::GREATER:
        case Token::GREATER_EQUAL:
        case Token::LESS:
        case Token::LESS_EQUAL:
        case Token::VAR:
        case Token::ERROR:
        case Token::END:
        default:
            return false;
    }
}

void Compiler::fatalError_(const char* fmt, ...) {
    hadFatalError_ = true;

    va_list args;
    va_start(args, fmt);
    errorAtVargs_(&currentToken_, fmt, args);
    va_end(args);
}

void Compiler::errorAtCurrent_(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    errorAtVargs_(&currentToken_, fmt, args);
    va_end(args);
}

void Compiler::errorAtPrevious_(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    errorAtVargs_(&previousToken_, fmt, args);
    va_end(args);
}

void Compiler::errorAt_(Token* token, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    errorAtVargs_(token, fmt, args);
    va_end(args);
}

void Compiler::errorAtVargs_(Token* token, const char* fmt, va_list args) {
    if( panicMode_ ) return;  // suppress errors after the first
    panicMode_ = true;

    if( scanner_.getPath() ){
        fprintf(stderr, "%s:%d:%d:", scanner_.getPath(), token->line, token->col);
    }else{
        for( int i = 0; i < token->col + 1; ++i ){
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^\n");
    }

    fprintf(stderr, " error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    // Reopen the stream to print the offending line, if available:
    InputStream * stream = scanner_.newCopyOfStream();
    if( stream ){
        // fast forward to the line
        for( int line = 0; line < token->line-1; ){
            char c = stream->next();
            if( c == '\n' ) line++;
            if( c == '\0' ) break;
        }
        // print the line
        char c = stream->next();
        while( c != '\n' && c != '\0' ){
            fprintf(stderr, "%c", c);
            c = stream->next();
        }
        fprintf(stderr, "\n");
        stream->close();
        delete stream;

        for( int i = 0; i < token->col - 2; ++i ){
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^\n\n");
    }

    hadError_ = true;
}

