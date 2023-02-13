---
title: file which
description: locate a file
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
flags:
- name: -q
  description: set quiet verbosity
- name: -v
  description: set loud verbosity
inlets:
  1st:
  - type: symbol
    description: file to locate using Pd's search-paths
outlets:
  1st:
  - type: list
    description: symbol path of located file and directory flag
  2nd:
  - type: float
    description: when there's an error creating the directory
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---
[file which] tries to locate the file in using Pd's search-paths and returns the resolved path.

notes:

- currently this only works for files, not for directories!

- currently only the first match is returned
