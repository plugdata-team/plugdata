---
title: file delete
description: remove files and directories
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
- file copy
- file move
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
    description: file or directory to be deleted
outlets:
  1st:
  - type: symbol
    description: the deleted path on success
  2nd:
  - type: bang
    description: if an error occurs
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---
NOTE: deleting destroys data. there is no confirmation dialog or anything of that kind.
