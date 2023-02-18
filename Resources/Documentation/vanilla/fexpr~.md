---
title: fexpr~
description: evaluation of audio signal expressions on a sample by sample basis
categories:
- object
pdcategory: vanilla, Signal Math, Logic
last_update: 0.51-3
see_also:
- +
- '>'
- sin
- +~
- block~
- value
- random
- array
- cos~
- wrap~
- abs~
- log~
- sqrt~
- pow~
arguments:
- description: expression including operators, functions, inlet types, float and symbols
  type: list
inlets:
  1st:
  - type: signal
    description: input to '$x1'
  nth:
  - type: float
    description: if inlet type is '$f#' or '$i#'
  - type: symbol
    description: if inlet type is '$s#' or '$i#'
  - type: signal
    description: if inlet type is '$x#'
outlets:
  nth:
  - type: signal
    description: expression result

methods:
  - type: set <list>
    description: set values for previous input/output values
  - type: stop/start
    description: stop/start computation
  - type: clear <symbol>
    description: clear input/output memory, optional symbol specifies a specific input (such as x1) or output (y1)

draft: false
---
{{< md_include "objects/expr-family.md" >}}
