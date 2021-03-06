---
layout: page
title: Back Language
permalink: /language/
order: 5
---

This page describes the Back programming language and the Ein stack machine.
Back is a stack-based postfix programming language inspired by FORTH, RPL,
Scheme, and Bash.  It runs on the Ein stack machine and can be stepped line by
line for debugging, executed alternately with callbacks during normal operation
(20 Hz or 20 instructions per second), or collapsed to execute segments of
instructions in an accelerated mode during which callbacks are not answered
(kHz range).

Central to Ein and Back are the **call stack** and the **data stack**. Both
stacks are collections of **words**, and each stack has at any given time a **top
element**.  One step in the Ein machine main loop consists of **popping** the top
word from the call stack and **executing** it. Words behave like functions when
executed, affecting some change on to the Ein state, the call stack, or the
data stack. Some words are more like data than functions in that they only push
individual words onto the data stack (`1` and `"something"`) or groups of words
onto the call stack (`( "hello" print )`), while others (like `+`) are more
obviously functions in that they consume words from the data stack, perform a
computation, and leave a result on the data stack.

### Argument Order

Many programming languages invoke functions by first naming the function and then listing its
arguments from left to right:

```
/* C code */
printf("Hello world.\n");
```

This is called **prefix order**.

In the same languages one can find functions which take their arguments on either side:

```
/* C code */
int a = b + c;
```

Here both `=` and `+` are written in **infix order**.

Ein uses **postfix order**, which means
that argments are listed first, or to the left, of the functions to which they belong: 

```
/* back code */
"Hello world." print
b c + "a" store
```

The reason for this is that words take their arguments from the data stack. In order for 
something to end up on the data stack, something must put it there, and it needs to be put
there before it is used.


### Data and Variables

<!--  

String Integer Double CompoundWord

`store`

`define`

`1 1 +`

compound words
good for storing lists and tuples

Reactive variables: truePose

--> 

### Elementary Operations

Basic data operations are available.

<!--  

```
+
-
*
/

```

--> 

### Control Words 

Back includes looping and condition constructs.


<!--  
ift
ifte
while
not

--> 

### Domain Words

Some words invoke complicated domain specific behaviors or routines.

<!--  
tableReconstructTakeScene
--> 




