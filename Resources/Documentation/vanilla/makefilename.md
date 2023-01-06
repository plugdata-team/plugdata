---
title: makefilename
description: format a symbol with a variable field
categories:
- object
pdcategory: General
arguments:
- description: format strring with specifiers (%c,  %d,  '%i,  %e,  %E,  %f,  %g,  %G,  %o,  %s,  %u,  %x,  %X
    and %p).
  type: symbol
inlets:
  1st:
  - type: float/symbol
    description: to replace format specifiers.
  - type: set <string>
    description: message replaces format.
outlets:
  1st:
  - type: symbol
    description: formatted symbol.
draft: false
---
The Makefilename object generates name symbols according to a format string,  for use as a series of filenames,  table names,  or whatnot. You can plug in a variable number or symbol by using different types in the string ("such as %s",  "%c",  "%d",  "%X and others). Each object can have only one variable pattern,  but you can cascade objects for multiple substitutions.

----------------------

### Possible printf pattern types.

- `%c` - character

  - This pattern allows you to convert from a float to a character (symbols are converted to 0) and insert it into a symbol.

- `%d` or `%i` - signed decimal integer

  - Both '%d' and '%i' are the same. This pattern allows you to insert a signed (negative or positive) decimal integer into a symbol (symbols are converted to 0). Floats are truncated.

- `%e` or `%E` - decimal floating point in scientific notation

  - This pattern allows you to insert a number with scientific notation into a symbol (symbols are converted to 0). The '%e' or '%E' pattern specify respectively whether the exponential character is lower (e) or upper (E) case.

- `%f` - decimal floating point

  - This pattern allows you to format a float into a symbol (symbols are converted to 0).

- `%g` or `%G` - numbers with or without scientific notation

  - This pattern allows you to insert a number with or without scientific notation into a symbol (symbols are converted to 0). The '%g' or '%G' pattern specify respectively whether the exponential character is lower (e) or upper (E) case. The scientific notation is only used if there's not enough digit resolution. The default precision is 6 digits and we'll see how to change that later.

  - Also, the decimal point is not included on whole numbers. This means that, unlike '%e' or '%E', not at all numbers are converted to scientific notation.

- `%o` - unsigned octal

  - This pattern allows you to insert an unsigned (only positive) octal integer into a symbol (symbols are converted to 0). Floats are truncated. Note that since this is an unsigned format, sending negative numbers doesn't work.

- `%s` - string

  - This pattern allows you to insert a symbol, but note it also works for float messages.

- `%u` - signed decimal integer

  - This pattern allows you to insert a symbol, but note it also works for float messages.

- `%x` or `%X` - unsigned hexadecimal

  - This pattern allows you to insert a signed (only positive) hexadecimal integer into a symbol (symbols are converted to 0). Floats are trunctaed. The '%x' or '%X' pattern specify respectively whether the the characters are lower or upper case. Note that since this is an unsigned format, sending negative numbers doesn't work.

- `%p` - pointer representation

  - This pattern formats to a platform specific pointer representation of an incoming symbol (floats are cast to int and also converted).

--------------------

### Flags.

The `+` flag prepends a plus sign for positive signed numeric types (%d/%i/%e/%E/%f/%g/%G):

````

[127 \
|
[makefilename %+d]
|
[+127 \

````

The `#` flag presents an alternate form of some numeric types. For "%o", the number is preceded by a "0". For "%x" and "%X", the number is preceded by "0x" (if %x) or "0X" (if %X). For %g and %G, the decimal point and zeroes are not removed for integers when not in scientific notation.

````

[1 \
|
[makefilename %#o]
|
[01 \

````


----------------

### Precision.

The precision field behaves differently according to the type (strings, integers of floats). The syntax of this field is specified by a `.` and is followed by the precision number.

````

[-18 \
|
[makefilename %.2f]
|
[-18.00 \

````

For symbol strings (%s), the precision sets a maximum character limit. Below, we have a maximum of 4 characters, hence, the symbol "abcde" gets truncated.

For integer types ('%d'/'%i'/'%o'/'%u'/'%x'/'%X'/'%p'), the precision field does not set a maximum number of characters. Instead, it sets a fixed number of digits and adds zeros to the left as a fill. This is slightly different than setting a width field with a '0' flag. The difference is only observed for numbers of different sign as below. Note how the width field will suppress a zero to include a "-" character.

For floats, the precision field sets the maximum number of digits to the right of the decimal point. Note that there's a default of 6 digits. Also note that this affects the resolution and can cause the number to be rounded.

----------------
