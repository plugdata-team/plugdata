---
title: file stat
description: get metainformation about a file/directory
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
    description: file or directory name
outlets:
  1st:
  - type: anything
    description: several messages for metainformation
  2nd:
  - type: bang
    description: if an error occurs
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---
[file stat] queries the filesystem about the given path, and outputs the collected data as a number of routable messages.
