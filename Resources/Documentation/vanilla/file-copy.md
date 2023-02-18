---
title: file copy
description: copy a file around
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
- file size
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
  - type: list
    description: source and destination
outlets:
  1st:
  - type: list
    description: source and destination on success
  2nd:
  - type: bang
    description: if an error occurs
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---
[file copy] duplicates the content of a file to a destination.
