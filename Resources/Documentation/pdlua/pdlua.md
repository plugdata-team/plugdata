---
title: pdlua

description: load externals written in Lua

categories:
 - object

pdcategory: pdlua, Logic

arguments:

inlets:
  1st:
  - type: load <symbol>
    description: load and run a '*.lua' file

outlets:

draft: false
---

[pdlua] registers a loader that allows Pd externals written in Lua (with the "*.pd_lua" extension) to be loaded.
