---
title: file
description: low-level file operations
categories:
- object
pdcategory: Misc
last_update: '0.52'
see_also:
- text
- array
- list
- file handle
- file define
- file mkdir
- file which
- file glob
- file stat
- file isfile
- file isdirectory
- file size
- file copy
- file move
- file delete
- file split
- file join
- file splitext
- file splitname
arguments:
- description: 'sets the function of [file], possible values: handle, define, mkdir,
    which, glob, stat, isfile, isdirectory, size, copy, move, delete, split, join,
    splitext and splitname. The default value is ''handle''.'
  type: symbol
flags:
- description: set quiet verbosity.
  flag: -q
- description: set loud verbosity.
  flag: -v
- description: file creation mode (user/group/other permissions) in octal.
  flag: -m
inlets:
  1st:
  - type: open <symbol>
    description: open a file.
  - type: float
    description: read number of bytes.
  - type: seek <list>
    description: seek file.
  - type: close
    description: close file.
  - type: verbose <float>
    description: set verbosity on or off.
  - type: creationmode <octal>
    description: restrict permissions of the to-be-created file.
  2nd:
  - type: symbol
    description: change the associated file-handle.
outlets:
  1st:
  - type: list
    description: data bytes.
  2nd:
  - type: bang
    description: if file can't be opened, end of the file is reached or a read error
      occurred.
  - type: seek <float>
    description: seek output.
methods:
- description: explicit Read-mode
  method: open <symbol> r
- description: open file for writing (Append mode)
  method: open <symbol> a
- description: open file for writing (Create (or trunCate) mode)
  method: open <symbol> c
draft: false
---
Short for "file handle"
