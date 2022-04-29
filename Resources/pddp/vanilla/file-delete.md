---
title: file delete
description: remove files and directories.
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
- file move
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
    description: file or directory to be deleted.
  - type: verbose <float>
    description: set verbosity on or off.
outlets:
  1st:
  - type: symbol
    description: the deleted path on success.
  2nd:
  - type: bang
    description: if an error occurs.
draft: false
---
NOTE: deleting destroys data. there is no confirmation dialog or anything of that kind.
