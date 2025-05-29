NOTE:  INVESTIGATION
        ON MASTER!!
- investigate what happens to local indexing with a deep function stack
    locals are relative to slots pointer, not stack root
    slots pointer is shifted around when calling functions.

- does close_upvalue ever need to close more than one upvalue?

        ????


- Is a valid solution to ensure there are no arithmatic (particularly left-operations) performed on expression block/if/for output? Does this still work for complex nested statements? Do we ever want to add results of expression-block/*s?
Test programs which should work:
var result = process({var a = 1; a+1}, {var b = 2; b+2});
print(result);
result = for i in 10 {  // [output] and i pushed to stack
    const m = { var a = 10; match(m, a) };  // push a, call match, pop a, push m
    if m {
        var i = 1;  // push i
        {
            const offset = -1
            m[i + offset]
        }
    }
};
print(result);

Instead: track stack impact of every opcode produced.
predict stack location of locals amongst temporary values.
Lox has strict order of [vars..., temps...] on the stack
Sigil can have a mix due to declaring locals mid expression.

Will this work for branches? branches with return?
NO, abandon this approach :(
Disallow defining locals mid expression :(
To work around, can we allow local at the if boundary?
result = for i in 10 {
    if const m = match(m, a) {
        m[0]
    }
};

As an experiment, can we do without locals, just style as above?
