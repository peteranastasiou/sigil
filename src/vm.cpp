
#include "vm.hpp"
#include "debug.hpp"
#include "compiler.hpp"
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
    return function->chunk.getLiteral(readByte());
}

ObjString * CallFrame::readString() {
    // look up literal from bytecode and cast to string:
    Value literal = readLiteral();
    assert(literal.isString());
    return literal.asObjString();
}

int CallFrame::chunkOffsetOf(uint8_t * addr) {
    // get distance of instruction address from the start of the chunk's code array:
    return (int)(addr - function->chunk.getCode());
}

Vm::Vm() {
    objects_ = nullptr;
    resetStack_();
}

Vm::~Vm() {
    freeObjects_();
}

InterpretResult Vm::interpret(char const * source) {
    // Compile the source string to a function
    Compiler compiler(this);
    ObjFunction * fn = compiler.compile(source);
    if( fn == nullptr )  return InterpretResult::COMPILE_ERR;

    // NOTE: not in lox!
    resetStack_();

    // put the function on the value stack
    push(Value::function(fn));

    // Make a new call frame
    call_(fn, 0);

    InterpretResult res = run_();
    if( res == InterpretResult::OK ){
        // assert nothing is left on the stack at the end of the script!
        assert(stackTop_ - stack_ == 0);
    }
    return res;
}

void Vm::registerObj(Obj * obj){
    obj->next = objects_;  // previous head
    objects_ = obj;        // new head
}

void Vm::deregisterObj(Obj * obj){
    // TODO
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
    if( fn.type != Value::Type::FUNCTION ){
        runtimeError_("Can only call functions.");
        return false;
    }
    return call_(fn.asObjFunction(), argCount);
}

bool Vm::call_(ObjFunction * fn, uint8_t argCount) {
    if( argCount != fn->arity ){
        runtimeError_("Expected %d arguments, but got %d.",
            fn->arity, argCount);
        return false;
    }

    if( frameCount_ >= FRAMES_MAX ){
        runtimeError_("Stack overflow.");
        return false;
    }

    CallFrame * frame = &frames_[frameCount_++];
    frame->function = fn;
    frame->ip = fn->chunk.getCode();
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
    Value bValue = pop();
    ObjString * b = bValue.toString(this);
    ObjString * a = pop().asObjString();
    push( Value::string(ObjString::concatenate(this, a, b)) );
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

    internedStrings_.debug();
    debugObjectLinkedList(objects_);

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

    // disasm.disassembleChunk(chunk, "Main");
    // printf("====\n");

#endif

    for(;;) {

#ifdef DEBUG_TRACE_EXECUTION
        printf("          stack: ");
        for( Value * slot = stack_; slot < stackTop_; slot++ ){
            printf("[ ");
            slot->print();
            printf(" ]");
        }
        printf("\n");

        disasm.disassembleInstruction(&frame->function->chunk,
            frame->chunkOffsetOf(frame->ip));
#endif

        uint8_t instr = frame->readByte();
        switch( instr ){
            case OpCode::LITERAL:{
                push(frame->readLiteral());
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
            case OpCode::POP_N: pop(frame->readByte()); break;
            case OpCode::DEFINE_GLOBAL_VAR:
            case OpCode::DEFINE_GLOBAL_CONST: {
                ObjString * name = frame->readString();
                bool isConst = instr==OpCode::DEFINE_GLOBAL_CONST;
                if( !globals_.add(name, {peek(0), isConst}) ){
                    runtimeError_("Redeclaration of variable '%s'.", name->get());
                    return InterpretResult::RUNTIME_ERR;
                }
                pop(); // Note: lox has this late pop as `set` might trigger garbage collection
                break;
            }
            case OpCode::GET_GLOBAL: {
                ObjString * name = frame->readString();
                Global global;
                if( !globals_.get(name, global) ){
                    runtimeError_("Undefined variable '%s'.", name->get());
                    return InterpretResult::RUNTIME_ERR;
                }
                push(global.value);
                break;
            }
            case OpCode::SET_GLOBAL: {
                ObjString * name = frame->readString();
                Global global;
                if( !globals_.get(name, global) ){
                    runtimeError_("Undefined variable '%s'.", name->get());
                    return InterpretResult::RUNTIME_ERR;
                }
                if( global.isConst ){
                    runtimeError_("Cannot redefine const variable '%s'.", name->get());
                    return InterpretResult::RUNTIME_ERR;
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
            case OpCode::EQUAL: {
                push(Value::boolean( pop().equals(pop()) ));
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
                if( peek(1).isString() ){ 
                    // implicitly convert second operand to string
                    concatenate_();

                }else if( peek(0).isNumber() && peek(1).isNumber() ){
                    double b = pop().as.number;
                    double a = pop().as.number;
                    push(Value::number( a + b ));
                }else{
                    runtimeError_("Invalid operands for +");
                    return InterpretResult::RUNTIME_ERR;
                }
                break;
            }
            case OpCode::NEGATE:{
                // ensure is numeric:
                if( !peek(0).isNumber() ){
                    runtimeError_("Operand must be a number");
                    return InterpretResult::RUNTIME_ERR;
                }

                push( Value::number(-pop().as.number) );
                break;
            }
            case OpCode::NOT:{
                push(Value::boolean(!isTruthy_(pop())));
                break;
            }
            case OpCode::PRINT:{
                pop().print();
                printf("\n");
                push(Value::nil());  // print returns nil
                break;
            }
            case OpCode::TYPE:{
                push(Value::typeId(pop().type));
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
                break;
            }
            case OpCode::RETURN:{
                // return value(s) of function:
                Value result = pop();

                // Check if we are returning from the top level script:
                if( --frameCount_ == 0 ){
                    pop();
                    return InterpretResult::OK;
                }

                // pop function literal & input params:
                stackTop_ = frame->slots;

                // put the result(s) back on the stack:
                push(result);

                // update the frame pointer to the caller:
                frame = &frames_[frameCount_ - 1];
                break;
            }
            default:{
                printf("Fatal error: unknown opcode %d\n", (int)instr);
                exit(1);
            }
        }
    }
}

void Vm::runtimeError_(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for( int i = frameCount_ - 1; i >= 0; i-- ){
        CallFrame * frame = &frames_[i];
        ObjFunction * fn = frame->function;
        int offset = frame->chunkOffsetOf(frame->ip - 1);
        fprintf(stderr, "[line %d] in ", fn->chunk.getLineNumber(offset));
        if( fn->name == nullptr ){
            fprintf(stderr, "script\n");
        }else{
            fprintf(stderr, "%s\n", fn->name->get());
        }
    }

    resetStack_();
}

void Vm::freeObjects_() {
    // iterate linked list of objects, deleting them
    Obj * obj = objects_;
    while( obj != nullptr ){
        Obj * next  = obj->next;
        delete obj;
        obj = next;
    }
}
