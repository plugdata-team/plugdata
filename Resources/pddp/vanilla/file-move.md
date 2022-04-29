---
title: file move
description: move a file to a new destination.
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
- file isdirectory
- file size
- file copy
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
  - type: list
    description: source and destination.
  - type: verbose <float>
    description: set verbosity on or off.
outlets:
  1st:
  - type: list
    description: source and destination on success.
  2nd:
  - type: bang
    description: if an error occurs.
draft: false
---
[file move] moves (renames) files
