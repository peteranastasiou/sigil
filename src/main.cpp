
#include "vm.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "inputstream/fileinputstream.hpp"
#include "inputstream/stringinputstream.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>


static void repl() {
    Vm vm;

    for( ;; ){
        char * line = readline("> ");
        if( line == nullptr ) return;  // Ctrl C or D

        if( strlen(line) > 0 ){
            add_history(line);
        }
        
        // TODO try to compile with "echo " on the front, then try to compile without.
        // This requires better error handling instead of printf everywhere! 
        vm.interpret(line);

        free(line);
    }
}

void readFile(const char* path) {
    FileInputStream stream;
    if( !stream.open(path) ){
        fprintf(stderr, "Could not open file '%s'\n", path);
        return;
    }
    bool rewind = true;
    for( ;; ) {
        char c = stream.next();
        if( c == '\0' ){
            return; // end of file
        }
        printf("%c", c);
        if( c == '{' && rewind ){
            stream.rewind(5);
            printf("|");
            rewind = false;
        }
    }
}

static void runFile(const char* path) {
    Vm vm;
    char* source;
    readFile(path);
    return;

    InterpretResult result = vm.interpret(source);
    free(source);

    if (result == InterpretResult::COMPILE_ERR) exit(65);
    if (result == InterpretResult::RUNTIME_ERR) exit(70);
}

int main(int argc, char const * argv[]) {
    if( argc == 1 ){
        repl();
    }else if( argc == 2 ){
        runFile(argv[1]);
    }else{
        fprintf(stderr, "Usage: pond [path]\n");
        return 64;
    }

    return 0;
}
