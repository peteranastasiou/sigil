#include "vm.hpp"
#include "value.hpp"
#include <assert.h>

int main() {
    printf("Running unit tests");
    Vm vm;
    vm.init();

    // Test push, pop, indexStack, stackSizeInFrame:
    {
        assert(vm.stackSizeInFrame() == 0);

        // Populate the stack
        vm.push(Value::nil());
        assert(vm.stackSizeInFrame() == 1);
        vm.push(Value::number(1.0f));
        assert(vm.stackSizeInFrame() == 2);
        vm.push(Value::boolean(true));
        assert(vm.stackSizeInFrame() == 3);
        // stack is now: nil, 1.0, true

        // Index forwards:
        assert(vm.indexStack(0).isNil());
        assert(vm.indexStack(1).isNumber());
        assert(vm.indexStack(2).isBoolean());

        // Index backwards:
        assert(vm.indexStack(-1).isBoolean());
        assert(vm.indexStack(-2).isNumber());
        assert(vm.indexStack(-3).isNil());

        // Pop the items off the stack
        assert(vm.pop().isBoolean());
        assert(vm.stackSizeInFrame() == 2);
        assert(vm.pop().isNumber());
        assert(vm.stackSizeInFrame() == 1);
        assert(vm.pop().isNil());
        assert(vm.stackSizeInFrame() == 0);
    }

    // TODO create and call native func

}