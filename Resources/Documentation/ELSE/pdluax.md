---
title: pdluax

description: load externals written in Lua

categories:
 - object

pdcategory: pdlua, ELSE, Data Management

arguments:
  - type: symbol
    description: source file name (without extension)

inlets:
outlets:

---

The [pdluax] object is similar to [pdlua] but you can load "*.pd_luax" files as arguments. It is less efficient but more flexible when developing or live-coding. You also need to load [pdlua] as a library first as with the [declare] object below. You can right click the object and ask to open the source file if your system has an application set to handle this extension.

