
#include "vm.hpp"
#include "debug.hpp"
#include "compiler.hpp"
#include "list.hpp"
#include "function.hpp"
#include "encode.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


uint16_t CallFrame::readUint16() {
    // TODO try one line: return encode::unpackUint16(ip += 2);
    ip += 2;
    return encode::unpackUint16(ip[-2], ip[-1]);
}

int16_t CallFrame::readInt16() {
    ip += 2;
    return encode::unpackInt16(ip[-2], ip[-1]);
}

Value CallFrame::readLiteral() {
    // look up literal from bytecode reference
    return closure->function->chunk.getLiteral(readByte());
}

ObjString * CallFrame::readString() {
    // look up literal from bytecode and cast to string:
    Value literal = readLiteral();
    assert(literal.isString());
    return literal.asObjString();
}

int CallFrame::chunkOffsetOf(uint8_t * addr) {
    // get distance of instruction address from the start of the chunk's code array:
    return (int)(addr - closure->function->chunk.getCode());
}

Vm::Vm() {
    compiler_ = nullptr;
    resetStack_();
}

void Vm::init() {
    mem_.init(this);
}

Vm::~Vm() {
}

InterpretResult Vm::interpret(char const * name, InputStream * stream) {
    // Compile the source string to a function
    compiler_ = new Compiler(&mem_);
    ObjFunction * fn = compiler_->compile(name, stream);
    if( fn == nullptr ){
        // Failed to compile
        // Done with compiler:
        delete compiler_;
        compiler_ = nullptr;
        return InterpretResult::COMPILE_ERR;
    }
    // NOTE: not in lox!
    resetStack_();

    // put the function on the value stack temporarily so that GC doesn't eat it
    push(Value::function(fn));

    ObjClosure * closure = new ObjClosure(&mem_, fn);
    pop(); // remove function from stack
    push(Value::closure(closure));

    // Make a new call frame
    call_(closure, 0);

    InterpretResult res = run_();
    if( res == InterpretResult::OK ){
        // assert nothing is left on the stack at the end of the script!
        assert(stackTop_ - stack_ == 0);
    }

    // Done with compiler:
    delete compiler_;
    compiler_ = nullptr;

    return res;
}

void Vm::gcMarkRoots() {
    // Mark all values in the stack:
    for( Value * value = stack_; value < stackTop_; value++ ){
#ifdef DEBUG_GC
        printf( "Mark value on stack:" );
        value->print(true);
        printf("\n");
#endif
        value->gcMark();
    }

    // Mark global values:
    globals_.gcMark();

    // Mark compiler-owned objects:
    compiler_->gcMarkRoots();
}

void Vm::push(Value value) {
    *stackTop_ = value;
    stackTop_++;
}

Value Vm::pop() {
    assert( stackTop_ != stack_ );
    stackTop_--;
    return *stackTop_;
}

void Vm::pop(int n) {
    assert( stackTop_ - n >= stack_ );
    stackTop_ -= n;
}

Value Vm::peek(int index) {
    return stackTop_[-1 - index];
}

bool Vm::defineGlobal(ObjString * name, Value v, bool isConst) {
    return globals_.add(name, {v, isConst});
}

bool Vm::getGlobal(ObjString * name, Global & g) {
    if( !globals_.get(name, g) ) {
        runtimeError_("Undefined variable '%s'.", name->get());
        return false;
    }
    return true;
}

bool Vm::setGlobal(ObjString * name, Value v) {
    Global global;
    if( !globals_.get(name, global) ){
        runtimeError_("Undefined variable '%s'.", name->get());
        return false;
    }
    if( global.isConst ){
        runtimeError_("Cannot redefine const variable '%s'.", name->get());
        return false;
    }
    if( !globals_.set(name, {v, false}) ){
        // Unexpected
        runtimeError_("Failed to set variable '%s'.", name->get());
        return false;
    }
    return true;
}

bool Vm::getUpvalue(uint8_t upvalueIdx, Value & v) {
    if( upvalueIdx >= frame_->closure->upvalues.size() ){
        runtimeError_("Upvalue out of range.");
        return false;
    }
    v = frame_->closure->upvalues[upvalueIdx]->get();
    return true;
}

bool Vm::setUpvalue(uint8_t upvalueIdx, Value v) {
    if( upvalueIdx >= frame_->closure->upvalues.size() ){
        runtimeError_("Upvalue out of range.");
        return false;
    }
    v = frame_->closure->upvalues[upvalueIdx]->get();
    return true;
}

bool Vm::isTruthy(Value value) {
    switch( value.type ){
        case Value::NIL:  return false;
        case Value::BOOL: return value.as.boolean;
        default:          return true;  // All other types are true!
    }
}

bool Vm::indexValue(Value value, Value index) {
    if( !index.isNumber() ){
        runtimeError_("Index must be a number");
        return false;
    }
    int i = (int) index.as.number;

    switch( value.type ){
    case Value::STRING:{
        char c;
        if( !value.asObjString()->get(i, c) ){
            runtimeError_("Index out of bounds: %i", i);
            return false;
        }
        push( Value::string(ObjString::newString(&mem_, &c, 1)) );
        return true;
    }
    case Value::LIST:{
        Value v;
        if( !value.asObjList()->get(i, v) ){
            runtimeError_("Index out of bounds: %i", i);
            return false;
        }
        push(v);
        return true;
    }
    default:
        runtimeError_("Cannot index %s", Value::typeToString(value.type));
        return false;
    }
}

bool Vm::binaryOp_(OpCode op) {
    if( !peek(0).isNumber() || !peek(1).isNumber() ){
        runtimeError_("Operands must be numbers.");
        return false;
    }

    double b = pop().as.number;
    double a = pop().as.number;
    switch( op ){
        case OpCode::GREATER:       push(Value::boolean( a > b )); break;
        case OpCode::GREATER_EQUAL: push(Value::boolean( a >= b )); break;
        case OpCode::LESS:          push(Value::boolean( a < b )); break;
        case OpCode::LESS_EQUAL:    push(Value::boolean( a <= b )); break;
        case OpCode::SUBTRACT:      push(Value::number( a - b )); break;
        case OpCode::MULTIPLY:      push(Value::number( a * b )); break;
        case OpCode::DIVIDE:        push(Value::number( a / b )); break;
        default: break;
    }
    return true;
}

bool Vm::compareIterator_() {
    Value aV = peek(1);
    Value bV = peek(0);
    if( !aV.isNumber() || !bV.isNumber() ){
        runtimeError_("Operands must be numbers.");
        return false;
    }

    double b = bV.as.number;
    double a = aV.as.number;
    double diff = b - a;
    if( abs(diff) < 1 ){
        // consider different within 1 as equal
        // we need this logic for for loops so they terminate correctly
        push(Value::number( 0 ));
    }else{
        push(Value::number( diff > 0 ? 1 : -1 ));
    }
    return true;
}

bool Vm::callValue_(Value fn, uint8_t argCount) {
    if( fn.type != Value::CLOSURE ){
        runtimeError_("Can only call functions.");
        return false;
    }
    return call_(fn.asObjClosure(), argCount);
}

bool Vm::call_(ObjClosure * closure, uint8_t argCount) {
    if( argCount != closure->function->numInputs ){
        runtimeError_("Expected %d arguments, but got %d.",
            closure->function->numInputs, argCount);
        return false;
    }

    if( frameCount_ >= FRAMES_MAX ){
        runtimeError_("Stack overflow.");
        return false;
    }

    CallFrame * frame = &frames_[frameCount_++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.getCode();
    frame->slots = stackTop_ - argCount - 1;
    return true;
}

void Vm::resetStack_() {
    stackTop_ = stack_;
    frameCount_ = 0;

    // Ready the first callframe for use before we start running
    frame_ = &frames_[0];
    frame_->slots = stack_;
}

InterpretResult Vm::run_() {
    // Grab the top call frame:
    frame_ = &frames_[frameCount_ - 1];

#ifdef DEBUG_TRACE_EXECUTION
    Disassembler disasm;

    // internedStrings_.debug();
    // debugObjectLinkedList(objects_);

    // printf("Literals:\n");
    // for( uint8_t i =0; i < chunk->numLiterals(); ++i ){
    //     printf(" %i [", i);
    //     Value v = chunk_->getLiteral(i);
    //     v.print();
    //     printf("]\n");
    // }
    // printf("Globals:\n");
    // globals_.debug();
    // printf("====\n");

    disasm.disassembleChunk(&frame_->closure->function->chunk, "Main");
    printf("====\n");

#endif

    for(;;) {

#ifdef DEBUG_TRACE_EXECUTION
       {
           printf("stack: ");
           for( Value * stackPos = stack_; stackPos < stackTop_; stackPos++ ){
               if ( stackPos != stack_ ) printf(" | ");
               if ( stackPos == frame_->slots ){
                  printf("[FP]"); // Frame Pointer
               }else{
                stackPos->print(true);
               }
           }
           printf("\n");
           printf("open-upvalues: ");
           ObjUpvalue * upvalue = mem_.getRootOpenUpvalue();
           while( upvalue != nullptr ){
               upvalue->print(true);
               upvalue = upvalue->getNextUpvalue();
               if ( upvalue != nullptr ) printf(" | ");
           }
           printf("\n");

           disasm.disassembleInstruction(&frame_->closure->function->chunk,
               frame_->chunkOffsetOf(frame_->ip));
        }
#endif

        OpCode instr = (OpCode)frame_->readByte();
        switch( instr ){
            case OpCode::PUSH_ZERO:{
                push(Value::number(0));
                break;
            }
            case OpCode::PUSH_ONE:{
                push(Value::number(1));
                break;
            }
            case OpCode::PUSH_TWO:{
                push(Value::number(2));
                break;
            }
            case OpCode::LITERAL:{
                push(frame_->readLiteral());
                break;
            }
            case OpCode::CLOSURE:{
                // Wrap the function literal into a closure:
                ObjFunction * function = frame_->readLiteral().asObjFunction();
                ObjClosure * closure = new ObjClosure(&mem_, function);
                push(Value::closure(closure));

                // Close over referenced Values (upvalues):
                for( int i = 0; i < function->numUpvalues; i++ ){
                    uint8_t isLocal = frame_->readByte();
                    uint8_t index = frame_->readByte();

                    closure->upvalues.push_back(
                        isLocal ?
                        // capture local value to upvalue:
                        ObjUpvalue::newUpvalue(&mem_, &frame_->slots[index]) :
                        // else, reference existing upvalue
                        frame_->closure->upvalues[index]
                    );
                }
                break;
            }
            case OpCode::NIL: push(Value::nil()); break;
            case OpCode::END: push(Value::end()); break;
            case OpCode::TRUE: push(Value::boolean(true)); break;
            case OpCode::FALSE: push(Value::boolean(false)); break;
            case OpCode::TYPE_BOOL: push(Value::typeId(Value::BOOL)); break;
            case OpCode::TYPE_FLOAT: push(Value::typeId(Value::NUMBER)); break;
            case OpCode::TYPE_FUNCTION: push(Value::typeId(Value::FUNCTION)); break;
            case OpCode::TYPE_STRING: push(Value::typeId(Value::STRING)); break;
            case OpCode::TYPE_TYPEID:   push(Value::typeId(Value::TYPEID)); break;
            case OpCode::POP: pop(); break;

            case OpCode::DEFINE_GLOBAL_VAR:
            case OpCode::DEFINE_GLOBAL_CONST: {
                ObjString * name = frame_->readString();
                bool isConst = instr==OpCode::DEFINE_GLOBAL_CONST;
                if( !defineGlobal(name, peek(0), isConst) ){
                    return runtimeError_("Redeclaration of variable '%s'.", name->get());
                }
                pop(); // Note: lox has this late pop as `set` might trigger garbage collection
                break;
            }
            case OpCode::GET_GLOBAL: {
                ObjString * name = frame_->readString();
                Global global;
                if( !getGlobal(name, global) ){
                    return InterpretResult::RUNTIME_ERR;
                }
                push(global.value);
                break;
            }
            case OpCode::SET_GLOBAL: {
                ObjString * name = frame_->readString();
                // just peek so the assignment can be used in an expression
                if( !setGlobal(name, peek(0)) ){
                    return InterpretResult::RUNTIME_ERR;
                }
                break;
            }
            case OpCode::GET_LOCAL: {
                // Get a value from the stack at the predicted location
                int8_t index = frame_->readByte();
                push(indexStack(index));
                break;
            }
            case OpCode::SET_LOCAL: {
                // Get a value from the stack at the predicted location
                Value dest = indexStack(frame_->readByte());
                dest = peek(0); // Not popping as assignment can be an expression
                break;
            }
            case OpCode::APPEND_LOCAL: {
                // Push to a value on the stack (if supported)
                Value dest = indexStack(frame_->readByte());
                Value src = pop();
                if( !dest.isList() ) {
                    return runtimeError_("Cannot append to %s type",
                        Value::typeToString(dest.type));
                }
                dest.asObjList()->append(src);
                break;
            }
            case OpCode::GET_UPVALUE: {
                uint8_t upvalueIdx = frame_->readByte();
                Value v;
                if( !getUpvalue(upvalueIdx, v) ){
                    return InterpretResult::RUNTIME_ERR;
                }
                push(v);
                break;
            }
            case OpCode::SET_UPVALUE: {
                uint8_t upvalueIdx = frame_->readByte();
                // Not popping as assignment can be an expression
                if( !setUpvalue( upvalueIdx, peek(0) ) ) {
                    return InterpretResult::RUNTIME_ERR;
                }
                break;
            }
            case OpCode::CLOSE_UPVALUE: {
                // close all upvalues to the top of the stack
                mem_.closeUpvalues(stackTop_ - 1);
                pop();
                break;
            }
            case OpCode::EQUAL: {
                push(Value::boolean( pop().equals(pop()) ));
                break;
            }
            case OpCode::COMPARE_ITERATOR: {
                compareIterator_(); // TODO remove
                break;
            }
            case OpCode::NOT_EQUAL: {
                push(Value::boolean( !pop().equals(pop()) ));
                break;
            }
            case OpCode::GREATER:
            case OpCode::GREATER_EQUAL:
            case OpCode::LESS:
            case OpCode::LESS_EQUAL:
            case OpCode::SUBTRACT:
            case OpCode::MULTIPLY:
            case OpCode::DIVIDE:{
                if( !binaryOp_(instr) ) return InterpretResult::RUNTIME_ERR;
                break;
            }
            case OpCode::ADD:{
                if( peek(0).isNumber() && peek(1).isNumber() ){
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(Value::number( a + b ));

                }else if( peek(1).isString() ){  // the first argument is second on stack
                    // implicitly convert second operand to string
                    Value bValue = pop();
                    ObjString * b = bValue.toString(&mem_);
                    ObjString * a = pop().asObjString();
                    push( Value::string(ObjString::concatenate(&mem_, a, b)) );

                }else if( peek(1).isList() && peek(0).isList() ){
                    // Concatenate two lists
                    ObjList * list = new ObjList(&mem_);
                    ObjList * b = pop().asObjList();
                    ObjList * a = pop().asObjList();
                    list->concat(a);
                    list->concat(b);
                    push( Value::list(list) );

                }else if( peek(1).isList() ){
                    // Copy a list and append a value
                    ObjList * list = new ObjList(&mem_);
                    Value b = pop();
                    ObjList * a = pop().asObjList();
                    list->concat(a);
                    list->append(b);
                    push( Value::list(list) );

                }else{
                    return runtimeError_("Invalid operands for '+': %s, %s",
                        Value::typeToString(peek(1).type), Value::typeToString(peek(0).type));
                }
                break;
            }
            case OpCode::NEGATE:{
                // ensure is numeric:
                if( !peek(0).isNumber() ){
                    return runtimeError_("Operand must be a number");
                }

                push( Value::number(-pop().as.number) );
                break;
            }
            case OpCode::NOT:{
                push(Value::boolean(!isTruthy(pop())));
                break;
            }
            case OpCode::ECHO:{
                pop().print(true);
                printf("\n");
                break;
            }
            case OpCode::PRINT:{
                pop().print(false);
                printf("\n");
                push(Value::nil());  // print returns nil
                break;
            }
            case OpCode::TYPE:{
                push(Value::typeId(pop().type));
                break;
            }
            case OpCode::MAKE_LIST:{
                ObjList * list = new ObjList(&mem_);
                uint8_t numEl = frame_->readByte();
                // populate list in reverse order from the value stack:
                for( int i = numEl-1; i >= 0; --i ){
                    if( !list->set(i, pop()) ){
                        return runtimeError_("Failed to initialise list.");
                    }
                }
                push(Value::list(list));
                break;
            }
            case OpCode::INDEX_GET:{
                Value index = pop();
                Value value = pop();
                if( !indexValue(value, index) ){
                    return InterpretResult::RUNTIME_ERR;
                }
                break;
            }
            case OpCode::INDEX_SET:{
                // TODO
                break;
            }
            case OpCode::JUMP:{
                int16_t offset = frame_->readInt16();
                frame_->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_TRUE:{
                uint16_t offset = frame_->readInt16();
                if( isTruthy(peek(0)) ) frame_->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_FALSE:{
                uint16_t offset = frame_->readInt16();
                if( !isTruthy(peek(0)) ) frame_->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_TRUE_POP:{
                uint16_t offset = frame_->readInt16();
                if( isTruthy(pop()) ) frame_->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_FALSE_POP:{
                uint16_t offset = frame_->readUint16();
                if( !isTruthy(pop()) ) frame_->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_ZERO:{
                uint16_t offset = frame_->readUint16();
                Value a = peek(0);
                if( a.type == Value::NUMBER && a.as.number == 0.0 ) frame_->ip += offset;
                break;
            }
            case OpCode::CALL: {
                uint8_t argCount = frame_->readByte();
                if( !callValue_(peek(argCount), argCount) ){
                    return InterpretResult::RUNTIME_ERR;
                }
                // now in a new frame:
                frame_ = &frames_[frameCount_ - 1];

#ifdef DEBUG_TRACE_EXECUTION
                disasm.disassembleChunk(
                    &frame_->closure->function->chunk,
                    frame_->closure->function->name->get());
                printf("====\n");
#endif
                break;
            }
            case OpCode::RETURN:{
                // return value(s) of function:
                Value result = pop();

                // close upvalues of function
                Value * newStackTop = frame_->slots;
                mem_.closeUpvalues(newStackTop);

                // Check if we are returning from the top level script:
                if( --frameCount_ == 0 ){
                    pop();
                    return InterpretResult::OK;
                }

                // pop function literal & input params:
                stackTop_ = newStackTop;

                // put the result(s) back on the stack:
                push(result);

                // update the frame pointer to the caller:
                frame_ = &frames_[frameCount_ - 1];
                break;
            }
            default:
                return runtimeError_("Fatal: unknown opcode %d\n", (int)instr);
        }
    }
}

int Vm::stackSizeInFrame() {
    return (int)(stackTop_ - frame_->slots);
}

Value Vm::indexStack(int8_t index) {
    if( index < 0 ){
        // Negative values index backwards from the top of the stack
        return stackTop_[-index];
    }else{
        // Positive values index forwards from the current stack frame (aka slots ptr)
        return frame_->slots[index];
    }
}

InterpretResult Vm::runtimeError_(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for( int i = frameCount_ - 1; i >= 0; i-- ){
        CallFrame * frame = &frames_[i];
        ObjFunction * fn = frame->closure->function;
        int offset = frame->chunkOffsetOf(frame->ip - 1);
        fprintf(stderr, "[line %d] in %s\n",
                fn->chunk.getLineNumber(offset),
                fn->name->get());
    }

    resetStack_();

    return InterpretResult::RUNTIME_ERR;
}
