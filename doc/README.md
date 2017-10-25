## Comments

```
# line comment
// line comment
/*
 block Comment
*/
```

## Variables

```
var a; // declaration and assign nil to a
var b = 42; // declaration and assign 42 to b
var c, d; // declaration c and d
```

## Value Types

```
a = nil; // value nil
a = true; // boolean true
a = false; // boolean false
a = sentinel; // a special value for nothing
a = 1234; // integer
a = 1234.5678; // number
a = "hello,world"; // string
a = 'hello,world'; // string
a = [1,2,3,4]; // array
a = {'a': 1, 'b': 2}; // dictionary
```

## Assignments

```
/* Assignment */
a = 42; // assign a to 42
b = true; // assign b to true
a, b = ['hello', 'world']; // a = 'hello', b = 'world'
a, b = [1, 2, 3, 4]; // a = 1, b = 2
a, b, c, d = [1, 2]; // a = 1, b = 2, c = nil, d = nil
```

## Conditional Statements

```
if (a == 42) {
    print('solved');
}
// else
if (a == 42) {
    print('solved');
} else {
    print('unsolved');
}
// else if
if (a == 42) {
    print('solved');
} else if (a == nil) {
    print('not started');
} else {
    print('unsolved');
}
```

## Loops

```
// while loop
while (a < 42) {
    a += 1;
}
// for loop
for (var i = 0; i < 42; i += 1) {
    a = i;
}
// for in loop
for (var i in [1,2,3,4]) {
    a = i;
}
```

## Functions

```
def hello1() {
    print('hello');
}
// parameters
def hello2(var name) {
    print('hello', name);
}
// keyword parameters
def hello3(var name = 'John') {
    print('hello', name);
}
// variable parameters
def hello4(var *names) {
    for (var name in names) {
        print('hello', name);
    }
}
// variable parameters with keywords
def hello5(var **names) {
    for (var name in names) {
        print('hello', names[name], name);
    }
}
```

## Function Calls

```
hello1(); // without arguments
hello2(a, b); // with fixed number arguments
hello3(name = a); // with keyword argument
hello4(*[a,b,c,d]); // with variable number arguments
hello5(**{a: b, c: d}); // with variable keyword arguments
```

## Classes

```
class Hello1() {
    var name; // class attribute
}
// init method
class Hello2() {
    def __init__(var name) {
        self.name = name;  // instance attribute
    }
}
```

## Class Instances

```
a = world1();
b = world2('John');
```

## Exceptions

```
// throw
throw Exception();
// try catch 
try {
    hello1();
} catch (TypeError e) {
    // catch specific exception
} catch (Exception e) {
    // catch all exception
} finally {
    // run whatever exception happen or not
}
```

## Libraries

```
import 'hello.lm'; // library name is `hello'

// import as
import 'hello.lm' as hi; // library name is `hi'

// import relative with current file
import './hello.lm'; // library name is `hello'

/* import will change every non-ANSI char to `_', such as */
import 'hello-library.lm' // library name is hello_library
```
