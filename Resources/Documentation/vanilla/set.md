---
title: set
description: set values in a scalar
categories:
- object
see_also: 
- pointer
- get
- append
- getsize
- setsize
- element
- struct
pdcategory: Accessing Data
last_update: '0.47'
inlets:
  1st:
  - type: set <symbol, symbol>
    description: if none or just one field is given, you can use 'set' to set struct name and field.
  "'n' :number of arguments set 'n' fields and we create 'n' inlets for them.":
  - type: float/symbol
    description: value in a scalar.
  2nd:
  - type: pointer
    description: a pointer to the scalar.
flags:
- flag:	"-symbol"
  description: so you can set symbol values.
arguments:
- type: symbol
  description: structure name.
- type: list
  description: symbols for field names (each creates an inlet).
draft: false
---
"Set" takes a pointer to a scalar in its rightmost inlet. The remaining inlets set numeric fields. Symbols are handled specially, as shown below. Arrays are accessed using the "element" object, and lists using "text" objects. Only the leftmost inlet is "hot".

To set fields whose values are symbols, give the set object the "-symbol" argument. (Unfortunately, you can't mix symbols and numbers in the same "set" object.)

You can use the template "-" as a wild card (this may be slower than if you use a specific template name). Also, if there are zero or one fields specified, you can send a "set" message to set a new template name and field name: