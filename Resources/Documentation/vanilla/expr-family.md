
Expr family objects by [Shahrokh Yadegari](http://yadegari.org/).

Based on original sources from IRCAM's jMax Released under BSD License.

The **expr** family is a set of C-like expression evaluation objects for the graphical music language Pure Data. It used to come as an 'extra' external, but it is now a built-in native object.

**expr** runs in control rate and evaluates C-like expressions. See below for the list of operators. Multiple expressions separated by semicolons can be defined in a single expr object and this results in multiple outlets (up to 100, each for each expression). Expressions are evaluated from right to left (which means that the bottom expression will be the first executed.) The number of inlets in expr are defined by variables that take a few different forms: **$i#**, **$i#** and **$s#** for 'integers', 'floats' and 'symbols' ('#' is an inlet number from 1 up to 100, ordered from left to right). As an example, we can have 3 inlets defined as "$i1", "$f2" and "$s3", where:

- **$i1** is the input from the first (left) inlet and is treated as an integer
- **$f2** is the input from the second (middle) inlet and is treated as a float
- **$s3** is the input from the third (middle) and expects a symbol (used to define array names)

Arrays and variables (defined using the 'value' object) can be accessed the same way one dimensional arrays are accessed in C; for example, "valx + 10" will be evaluated to the value of variable 'valx' + 10 and "tabname[5]" will be evaluated to be the 5th element of an array named "tabname". As shown above, the name of the arrays can be given by an input; for example "$s2[5]" will be evaluated to be the 5 element of the array whose symbol has been passed in inlet 2.

Type conversion from a float input to an integer is done automatically if the inlet is defined as an integer. Conversely, you can convert it explicitly by using functions (see below for the list of functions).

**expr~** is designed to efficiently combine signal and control stream processing by vector operations on the basis of the audio block size. The operations, functions, and syntax for **expr~** is just like expr with the addition of the $v# variable for signal vector input (also numbered up to 100). The **'$v'** is needed at least for the first and main input, so:

- **$v1** - means the first inlet is a signal input
- **$i2** - means the second inlet is an integer input
- **$f2** - means the third inlet is a float input
- **$s4** - means the fourth inlet is a symbol input

The result of an expression from expr~ is also an audio signal and multiple expressions up to 1000 can also be defined via semicolons.

Note for MSP users: Currently in the MSP version all signal inputs should come first followed by other types of inlet. (There seems to be no way of mixing signal and other types of inlets in their order in Max/MSP, if you know otherwise, please let me know.) This means that signal inlets cannot be mixed with other types of inlets. For example, "expr~ $v1$f2$v3 " is not legal. The second and third inlet should be switched and "expr~ $v1$v2$f3" should be used. In Pd you can mix them in any way you want.

The **fexpr~** object provides a flexible mechanism for building FIR and IIR filters by evaluating expressions on a sample by sample basis and providing access to prior samples of the input and output audio streams. When fractional offset is used, **fexpr~** uses linear interpolation to determine the value of the indexed sample. The operations, functions, and syntax for expr~ is just like expr with the addition of **$x#** and **$y#** variables. **fexpr~** can access previous input and output samples up to the block size (64 by default).

**$x#** is used to denote a signal input whose samples we would like to access. The syntax is $x followed by '#' (the inlet number up to 100) and the samples indexed by brackets, for example $x1[-1] specifies the previous sample of the first inlet. Therefore, if we are to build a simple filter which replaces every sample by the average of that sample and its previous one, we would use " **fexpr~ ($x1[0]+$x1[-1])/2** ". For ease of when the brackets are omitted, the current sample is implied, so we can write the previous filter expression as follows: " **fexpr~ ($x1+$x1[-1])/2** ". To build IIR filters **$y#** is used to access the previous output samples indexed from -1 inside brackets. Note now that '#' here is used to define the outlet number.

- **$x1[n]** - means the first inlet is a signal input and 'n' is an index from 0 to -block size
- **$y1[n]** - is used to access output samples from the first expression and 'n' is an index from -1 to -block size
- **$i2** - means the second inlet is an integer input
- **$f2** - means the third inlet is a float input
- **$s4** - means the fourth inlet is a symbol input


------------------

### The operators expr, expr~ and fexpr~ support (listed from highest precedence to lowest) are as follows:

|Operator |Description|
|:----|----:|
|~      |One's complement|
|*      |Multiply|
|/      |Divide|
|%      |Modulo|
|+      |Add|
|-      |Subtract|
|\<\<   |Shift Left|
|\>\>   |Shift Right|
|<      |Less than (boolean)|
|<=     |Less than or equal (boolean)|
|>      |Greater than (boolean)|
|>=     |Greater than or equal (boolean)|
|==     |Equal (boolean)|
|!=     |Not equal (boolean)|
|&      |Bitwise And|
|^      |Exclusive Or|
|\|     |Bitwise Or|
|&&     |Logical And (boolean)|
|\|\|   |Logical Or (boolean)|

### The supported functions for expr, expr~ and fexpr~ are:

|Functions |# of Args |Description|
|:---|:---|:--- |
|if () |3 |conditional - if (condition, IfTrue-expr, IfFalse-expr) - in expr~ if 'condition' is a signal, the result will be determined on sample by sample basis (added in version 0.4)v|
|int () |1 |convert to integer|
|rint () |1 |round a float to a nearby integer|
|float () |1 |convert to float|
|min () |2 |minimum|
|max () |2 |maximum|
|abs() |1 |absolute value (added in version 0.3)|
|if() |3 |conditional - if (condition, IfTrue-expr, IfFalse-expr) - in expr~ if 'condition' is a signal, the result will be determined on sample by sample basis (added in version 0.4)|
|isinf() |1 |is the value infinite (added in version 0.4)|
|finite() |1 |is the value finite (added in version 0.4)|
|isnan |1 |is the value non a number (added in version 0.4)|
|copysign() |1 |copy sign of a number(added in version 0.4)|
|imodf |1 |get signed integer value from floating point number(added in version 0.4)|
|modf |1 |get signed fractional value from floating-point number(added in version 0.4)|
|drem |2 |floating-point remainder function (added in version 0.4)|


### power functions

|Functions |# of Args |Description|
|:---|:---|:--- |
|pow () |2 |raise to the power of {e.g., pow(x,y) is x to the power of y}|
|sqrt () |1 |square root|
|exp() |1 |e raised to the power of the argument {e.g., exp(5.2) is e raised to the power of 5.2}|
|ln() and log() |1 |natural log|
|log10() |1 |log base 10|
|fact() |1 |factorial|
|erf() |1 |error function (added in version 0.4)|
|erfc() |1 |complementary error function (added in version 0.4)|
|cbrt() |1 |cube root (added in version 0.4)|
|expm1() |1 |exponential minus 1 (added in version 0.4)|
|log1p() |1 |logarithm of 1 plus (added in version 0.4)|
|ldexp() |1 |multiply floating-point number by integral power of 2 (added in version 0.4)|

### Trigonometric

|Functions |# of Args |Description|
|:---|:---|:--- |
|sin() |1 |sine|
|cos() |1 |cosine|
|tan() |1 |tangent|
|asin() |1 |arc sine|
|acos() |1 |arc cosine|
|atan() |1 |arc tangent|
|atan2() |2 |arc tangent of 2 variables|
|sinh() |1 |hyperbolic sine|
|cosh() |1 |hyperbolic cosine|
|tanh() |1 |hyperbolic tangent|
|asinh() |1 |inverse hyperbolic sine|
|acosh() |1 |inverse hyperbolic cosine|
|atan() |1 |inverse hyperbolic tangent|
|floor() |1 |largest integral value not greater than argument (added in version 0.4)|
|ceil() |1 |smallest integral value not less than argument (added in version 0.4)|
|fmod() |1 |floating-point remainder function (added in version 0.4)|

### Table Functions

|Functions |# of Args |Description|
|:---|:---|:--- |
|size() |1 |size of a table|
|sum() |1 |sum of all elements of a table|
|Sum() |3 |sum of elements of a specified boundary of a table|
|avg() |1 |averages all elements of a table|
|Avg() |3 |averages elements of a specified boundary of a table|

### Acoustics

|Functions |# of Args |Description|
|:---|:---|:--- |
|mtof() |1 |convert MIDI pitch to frequency in Hz|
|ftom() |1 |convert frequency in Hz to MIDI pitch|
|dbtorms() |1 |convert db to rms|
|rmstodb() |1 |convert rms to db|
|powtodb() |1 |convert power to db|
|dbtopow() |1 |convert db to power|

--------------------------

### CHANGELOG:

#### New Additions in version 0.57

- fixed a bug in fact().
- fact() (factorial) now calculates and returns its value in double
- fixed the bad lvalue bug - "4 + 5 = 3" was not caught before
Added mtof(), mtof(), dbtorms(), rmstodb(), powtodb(), dbtopow()

#### New Additions in version 0.56

- Fexpr~ now accepts a float in its first input.
- Added avg() and Avg() back to the list of functions

#### New Additions in version 0.55

- Expr, expr~, and fexpr~ are now built-in native objects.
- The arrays now redraw after a store into one of their members
- ex_if() (the "if()" function is reworked to only evaluate either the left or the right args depending on the truth value of the condition. However, if the condition is an audio vector, both the left and the right are evaluated regardless.
- priority of ',' and '=' was switched to fix the bug of using store "=" in functions with multiple arguments, which caused an error during execution.
- The number of inlet and outlets (MAX_VARS) is now set at 100

#### New Additions in version 0.5

- Expr, expr~, and fexpr~ are now built-in native objects.
- minor fixes/improvements.

#### New Additions in version 0.4

- Expr, expr~, and fexpr~ now support multiple expressions separated by semicolons which results in multiple outlets.
- Variables are supported now in the same way they are supported in C. - Variables have to be defined with the "value" object prior to execution.
- A new if function if (condition-expression, IfTrue-expression, IfFalse-expression) has been added.
- New math functions added.
- New shorthand notations for fexpr~ have been added.
  - $x ->$x1[0] $x# -> $x#[0]
  - $y = $y1[-1] and $y# = $y#[-1]
- New 'set' and 'clear' methods were added for fexpr~
  - clear - clears all the past input and output buffers
  - clear x# - clears all the past values of the #th input
  - clear y# - clears all the past values of the #th output
  - set x# val-1 val-2 ... - sets as many supplied value of the #th input; e.g., "set x2 3.4 0.4" - sets x2[-1]=3.4 and x2[-2]=0.4
  - set y# val-1 val-2 ... - sets as many supplied values of the #th output; e.g, "set y3 1.1 3.3 4.5" - sets y3[-1]=1.1 y3[-2]=3.3 and y3[-3]=4.5;
  - set val val ... - sets the first past values of each output; e.g., e.g., "set 0.1 2.2 0.4" - sets y1[-1]=0.1, y2[-1]=2.2, y3[-1]=0.4

---------------
