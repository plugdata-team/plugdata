---
title: pdluax

description: load externals written in Lua

categories:
 - object

pdcategory: pdlua, Logic

arguments:
  - type: symbol
    description: source file name (without extension)

inlets:
outlets:

draft: false
---

The [pdluax] object is similar to [pdlua] but you can use "*.pd_luax" instead and load them as arguments. It is less efficient but more flexible when developing or live-coding. You also need to load [pdlua] as a library first as with the [declare] object above.
