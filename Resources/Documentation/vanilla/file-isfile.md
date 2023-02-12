---
title: file isfile
description: check if path is an existing regular file
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
- file isdirectory
- file size
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
    description: true <1> of false <0>
  2nd:
  - type: bang
    description: if an error occurs
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---

