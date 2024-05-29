
#include "vm.hpp"
#include "debug.hpp"
#include "compiler.hpp"
#include "list.hpp"
#include "function.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


uint16_t CallFrame::readUint16() {
    ip += 2;
    return (uint16_t)((ip[-2] << 8) | ip[-1]);
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

bool Vm::binaryOp_(uint8_t op) {
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

bool Vm::isTruthy_(Value value) {
    switch( value.type ){
        case Value::NIL:  return false;
        case Value::BOOL: return value.as.boolean;
        default:          return true;  // All other types are true!
    }
}

void Vm::concatenate_() {
}

bool Vm::indexGet_() {
    Value index = pop();
    Value value = pop();

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

void Vm::resetStack_() {
    stackTop_ = stack_;
    frameCount_ = 0;
}

InterpretResult Vm::run_() {
    // Grab the top call frame:
    CallFrame * frame = &frames_[frameCount_ - 1];

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

    disasm.disassembleChunk(&frame->closure->function->chunk, "Main");
    printf("====\n");

#endif

    for(;;) {

#ifdef DEBUG_TRACE_EXECUTION
       {
           printf("stack: ");
           for( Value * stackPos = stack_; stackPos < stackTop_; stackPos++ ){
               if ( stackPos != stack_ ) printf(" | ");
               if ( stackPos == frame->slots ){
                  printf("SF: "); // Stack Frame
               }
               stackPos->print(true);
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

           disasm.disassembleInstruction(&frame->closure->function->chunk,
               frame->chunkOffsetOf(frame->ip));
        }
#endif

        uint8_t instr = frame->readByte();
        switch( instr ){
            case OpCode::LITERAL:{
                push(frame->readLiteral());
                break;
            }
            case OpCode::CLOSURE:{
                // Wrap the function literal into a closure:
                ObjFunction * function = frame->readLiteral().asObjFunction();
                ObjClosure * closure = new ObjClosure(&mem_, function);
                push(Value::closure(closure));

                // Close over referenced Values (upvalues):
                for( int i = 0; i < function->numUpvalues; i++ ){
                    uint8_t isLocal = frame->readByte();
                    uint8_t index = frame->readByte();

                    closure->upvalues.push_back(
                        isLocal ? 
                        // capture local value to upvalue:
                        ObjUpvalue::newUpvalue(&mem_, &frame->slots[index]) :
                        // else, reference existing upvalue
                        frame->closure->upvalues[index]
                    );
                }
                break;
            }
            case OpCode::NIL: push(Value::nil()); break;
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
                ObjString * name = frame->readString();
                bool isConst = instr==OpCode::DEFINE_GLOBAL_CONST;
                if( !globals_.add(name, {peek(0), isConst}) ){
                    return runtimeError_("Redeclaration of variable '%s'.", name->get());
                }
                pop(); // Note: lox has this late pop as `set` might trigger garbage collection
                break;
            }
            case OpCode::GET_GLOBAL: {
                ObjString * name = frame->readString();
                Global global;
                if( !globals_.get(name, global) ){
                    return runtimeError_("Undefined variable '%s'.", name->get());
                }
                push(global.value);
                break;
            }
            case OpCode::SET_GLOBAL: {
                ObjString * name = frame->readString();
                Global global;
                if( !globals_.get(name, global) ){
                    return runtimeError_("Undefined variable '%s'.", name->get());
                }
                if( global.isConst ){
                    return runtimeError_("Cannot redefine const variable '%s'.", name->get());
                }
                globals_.set(name, {peek(0), false});
                // don't pop: the assignment can be used in an expression
                break;
            }
            case OpCode::GET_LOCAL: {
                // local is already on the stack at the predicted index:
                uint8_t slot = frame->readByte();
                push(frame->slots[slot]);
                break;
            }
            case OpCode::SET_LOCAL: {
                uint8_t slot = frame->readByte();  // stack position of the local
                frame->slots[slot] = peek(0);      // note: no pop: assignment can be an expression
                break;
            }
            case OpCode::GET_UPVALUE: {
                uint8_t upvalueIdx = frame->readByte();
                push( frame->closure->upvalues[upvalueIdx]->get() );
                break;
            }
            case OpCode::SET_UPVALUE: {
                uint8_t upvalueIdx = frame->readByte();
                frame->closure->upvalues[upvalueIdx]->set( peek(0) );
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
            case OpCode::EQUAL_PEEK: {
                push(Value::boolean( peek(0).equals(peek(1)) ));
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
                if( peek(1).isString() ){  // the first argument is second on stack
                    // implicitly convert second operand to string
                    Value bValue = pop();
                    ObjString * b = bValue.toString(&mem_);
                    ObjString * a = pop().asObjString();
                    push( Value::string(ObjString::concatenate(&mem_, a, b)) );

                }else if( peek(0).isList() && peek(1).isList() ){
                    ObjList * b = pop().asObjList();
                    ObjList * a = pop().asObjList();
                    push( Value::list(new ObjList(&mem_, a, b)) );

                }else if( peek(0).isNumber() && peek(1).isNumber() ){
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(Value::number( a + b ));
                }else{
                    return runtimeError_("Invalid operands for +");
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
                push(Value::boolean(!isTruthy_(pop())));
                break;
            }
            case OpCode::ECHO:{
                pop().print(true);
                printf("\n");
                push(Value::nil());  // echo returns nil
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
                uint8_t numEl = frame->readByte();
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
                if( !indexGet_() ){
                    return InterpretResult::RUNTIME_ERR;
                }
                break;
            }
            case OpCode::INDEX_SET:{
                // TODO
                break;
            }
            case OpCode::JUMP:{
                uint16_t offset = frame->readUint16();
                frame->ip += offset;  // jump forwards
                break;
            }
            case OpCode::LOOP:{
                uint16_t offset = frame->readUint16();
                frame->ip -= offset;  // jump backwards
                break;
            }
            case OpCode::JUMP_IF_TRUE:{
                uint16_t offset = frame->readUint16();
                if( isTruthy_(peek(0)) ) frame->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_FALSE:{
                uint16_t offset = frame->readUint16();
                if( !isTruthy_(peek(0)) ) frame->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_TRUE_POP:{
                uint16_t offset = frame->readUint16();
                if( isTruthy_(pop()) ) frame->ip += offset;
                break;
            }
            case OpCode::JUMP_IF_FALSE_POP:{
                uint16_t offset = frame->readUint16();
                if( !isTruthy_(pop()) ) frame->ip += offset;
                break;
            }
            case OpCode::CALL: {
                uint8_t argCount = frame->readByte();
                if( !callValue_(peek(argCount), argCount) ){
                    return InterpretResult::RUNTIME_ERR;
                }
                // now in a new frame:
                frame = &frames_[frameCount_ - 1];

#ifdef DEBUG_TRACE_EXECUTION
                disasm.disassembleChunk(
                    &frame->closure->function->chunk, 
                    frame->closure->function->name->get());
                printf("====\n");
#endif
                break;
            }
            case OpCode::RETURN:{
                // return value(s) of function:
                Value result = pop();

                // close upvalues of function
                Value * newStackTop = frame->slots;
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
                frame = &frames_[frameCount_ - 1];
                break;
            }
            default:
                return runtimeError_("Fatal: unknown opcode %d\n", (int)instr);
        }
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
