Sigil is a garbage-collected, bytecode-interpreted toy scripting language.

It was created to learn and explore the workings of programming languages but mainly just for fun.

The language design is my own and the implementation draws from [Crafting Interpreters](https://craftinginterpreters.com/).

Source code is compiled and fed to a stack-based virtual machine interpreter.

# Usage
Sigil has one dependency: readline.

`make`

`./bin/sigil` for an interactive prompt

`./bin/sigil [filename.sigil]` to run a script

# Features
### Variables
Declaration is made with `var` (mutable) or `const` (immutable).
Scoped variables are stored on the stack, top-level variables are globals in the heap.
Lists, numbers and strings are supported
```
var ls = [1, "2", 3];
const a = 0.5;
print(a + ls[0]);  # 1.5
print(type(ls));   # list
```

### Closures
Named and anonymous functions with variable capture.
```
fn greet(name) {
  return fn() {
    print("Hello " + name);
  };
}

const g = greet("World");

print(g());  # Hello World
```

### Statement form and expression forms for all control structures
Taking a leaf from the functional playbook, this means more composable logic, and less temporary variables.

Inspired by Rust, the last non-semicoloned statement in a block is captured/returned.

Output of `if` statements can be captured (meaning no need for a specific ternary syntax).
```
const absn = if n > 0 { n } else { -n };
```

The expression form of `for` and `while` capture each iteration's result in a list (not implemented yet!)
```
const doubles = for i in 1:10 { print(i); 2*i };

const evens = for i in 0:10 { if i % 2 == 0 { i } else { continue } };  # filter using continue
```

Implicit return is also available to functions:
```
const sq = fn(i) { var n = i; n*n };
```

### Roadmap:
 - Imports (in progress)
 - Expression forms for `for` and `while`
 - Exceptions
 - Objects
 - Extensible C interface for libraries
