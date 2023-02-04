---
title: pack2
description: combine atoms

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: the number of elements defines the number of inlets. A float or a symbol will define an initial value
  default: two floats set to '0'

inlets:
  1st:
  - type: bang
    description: outputs the stored list of elements

outlets:
  1st:
  - type: anything
    description: the message composed of the given elements

methods:
  - type: set <anything>
    description: the elements of a message update the inlet's value they are connected to and the subsequent inlets according to the remaining elements - it doesn't force an output 
  - type: anything
    description: same as "set", but it forces an output

---

[pack2] is kinda similar to Pd Vanilla's [pack], but any inlet triggers the output (though a "set" message avoids the output). It can also initialize a symbol value and it doesn't create a list selector for you. Moreover, you can change many elements with a list message and an element that was initialized as a float can become a symbol and vice versa.

