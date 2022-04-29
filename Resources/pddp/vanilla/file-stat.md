---
title: file stat
description: get metainformation about a file/directory
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
  - type: anything
    description: several messages for metainformation.
  2nd:
  - type: bang
    description: if an error occurs.
draft: false
---
[file stat] queries the filesystem about the given path, and outputs the collected data as a number of routable messages.
