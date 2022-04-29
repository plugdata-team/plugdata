---
title: file isdirectory
description: check if path is an existing directory
categories:
- object
pdcategory: Misc
last_update: '0.52'
see_also:
- text
- array
- list
- file
- file define
- file handle
- file mkdir
- file which
- file glob
- file stat
- file isfile
- file size
- file copy
- file move
- file delete
- file split
- file join
- file splitext
- file splitname
flags:
- description: set quiet verbosity.
  flag: -q
- description: set loud verbosity.
  flag: -v
inlets:
  1st:
  - type: symbol
    description: file or directory name.
  - type: verbose <float>
    description: set verbosity on or off.
outlets:
  1st:
  - type: float
    description: true <1> of false <0>.
  2nd:
  - type: bang
    description: if an error occurs.
draft: false
---

