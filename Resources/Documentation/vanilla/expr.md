---
title: expr
description: evaluation of control data expressions
categories:
- object
pdcategory: vanilla, Data Math, Logic
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
  - type: bang
    description: evaluate expression
  - type: float
    description: if inlet type is '$f1' or '$i1'
  - type: symbol
    description: if inlet type is '$s1' or '$i1'
  nth:
  - type: float
    description: if inlet type is '$f#' or '$i#'
  - type: symbol
    description: if inlet type is '$s#' or '$i#'

outlets:
  nth:
  - type: float
    description: expression result

draft: false
---
{{< md_include "objects/expr-family.md" >}}
