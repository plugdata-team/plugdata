---
title: pack2
description: combine atoms

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: the number of elements = the number of inlets. <f> or <symbol> — initial value
  default: two floats set to '0'

inlets:
  1st:
  - type: bang
    description: outputs the stored list of elements
  - type: float/symbol
    description: a float or symbol to set to slot 0 and output
  - type: set <anything>
    description: a float or symbol to set to slot 0
  nth:
  - type: float/symbol
    description: a float or symbol to set to slot $nth and output
  - type: set <anything>
    description: a float or symbol to set to slot $nth
outlets:
  1st:
  - type: list
    description: the message composed of the given elements

draft: false
---

[pack2] is kinda similar to Pd Vanilla's [pack], but any inlet triggers the output (though a "set" message avoids the output). It can also initialize a symbol value and it doesn't create a list selector for you. Moreover, you can change many elements with a list message and an element that was initialized as a float can become a symbol and vice versa.
