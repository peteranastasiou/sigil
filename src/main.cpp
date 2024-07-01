
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
    vm.init();

    // Use for debugging:
    // const char * line = "var a = \"abc\";";
    // StringInputStream s(line);
    // vm.interpret("(debug)", &s);

    for( ;; ){
        char * line = readline("> ");
        if( line == nullptr ) return;  // Ctrl C or D

        if( strlen(line) > 0 ){
            add_history(line);
        }

        StringInputStream stream(line);

        // TODO try to compile with "echo " on the front, then try to compile without.
        // This requires better error handling instead of printf everywhere!
        vm.interpret("(stdin)", &stream);

        free(line);
    }
}

static void runFile(const char* path) {
    FileInputStream stream;
    if( !stream.open(path) ){
        fprintf(stderr, "Could not open file '%s'\n", path);
        exit(74);
    }

    Vm vm;
    vm.init();
    InterpretResult result = vm.interpret(path, &stream);

    if (result == InterpretResult::COMPILE_ERR) exit(65);
    if (result == InterpretResult::RUNTIME_ERR) exit(70);
}

int main(int argc, char const * argv[]) {
    if( argc == 1 ){
        repl();
    }else if( argc == 2 ){
        runFile(argv[1]);
    }else{
        fprintf(stderr, "Usage: sigil [path]\n");
        return 64;
    }

    return 0;
}
