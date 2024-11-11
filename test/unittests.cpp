#include "vm.hpp"
#include "value.hpp"
#include <assert.h>
#include <iostream>

#define testEquals(a, b) do { \
    if( !testEquals_((a), (b), __LINE__) ) return 1; \
} while(0)

#define testTrue(a) do { \
    if( !testTrue_((a), __LINE__) ) return 1; \
} while(0)

template <typename T>
bool testEquals_(T a, T b, int line) {
    if(a != b) {
        std::cout << "Failed on line " << line <<
            ": " << a << "!=" << b << std::endl;
        return false;
    }
    return true;
}

bool testTrue_(bool b, int line) {
    if(!b) {
        std::cout << "Failed on line " << line << std::endl;
        return false;
    }
    return true;
}

int main() {
    printf("Running unit tests\n");
    Vm vm;
    vm.init();

    // Test push, pop, indexStack, stackSizeInFrame:
    {
        testEquals(vm.stackSizeInFrame(), 0);

        // Populate the stack
        vm.push(Value::nil());
        testEquals(vm.stackSizeInFrame(), 1);
        vm.push(Value::number(1.0f));
        testEquals(vm.stackSizeInFrame(), 2);
        vm.push(Value::boolean(true));
        testEquals(vm.stackSizeInFrame(), 3);
        // stack is now: nil, 1.0, true

        // Index forwards:
        testTrue(vm.indexStack(0).isNil());
        testTrue(vm.indexStack(1).isNumber());
        testTrue(vm.indexStack(2).isBoolean());

        // Index backwards:
        testTrue(vm.indexStack(-1).isBoolean());
        testTrue(vm.indexStack(-2).isNumber());
        testTrue(vm.indexStack(-3).isNil());

        // Pop the items off the stack
        testTrue(vm.pop().isBoolean());
        testEquals(vm.stackSizeInFrame(), 2);
        testTrue(vm.pop().isNumber());
        testEquals(vm.stackSizeInFrame(), 1);
        testTrue(vm.pop().isNil());
        testEquals(vm.stackSizeInFrame(), 0);

        printf("SUCCESS\n");
    }

    // TODO create and call native func

}