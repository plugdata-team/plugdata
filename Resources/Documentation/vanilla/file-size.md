---
title: file size
description: get size of a file
categories:
- object
pdcategory: vanilla, File Management
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
- file copy
- file move
- file delete
- file split
- file join
- file splitext
- file splitname
flags:
- name: -q
  description: set quiet verbosity
- name: -v
  description: set loud verbosity
inlets:
  1st:
  - type: symbol
    description: file or directory name
outlets:
  1st:
  - type: float
    description: file size in bytes (0 for directories)
  2nd:
  - type: bang
    description: if an error occurs
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---

