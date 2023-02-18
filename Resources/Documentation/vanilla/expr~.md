---
title: expr~
description: evaluation of audio signal expressions on a vector by vector basis
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
    description: input to '$v1'
  nth:
  - type: float
    description: if inlet type is '$f#' or '$i#'
  - type: symbol
    description: if inlet type is '$s#' or '$i#'
  - type: signal
    description: if inlet type is '$v#'
outlets:
  nth:
  - type: signal
    description: expression result

draft: false
---
{{< md_include "objects/expr-family.md" >}}
