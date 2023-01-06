---
title: file mkdir
description: create directories
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
flags:
- description: set quiet verbosity.
  flag: -q
- description: set loud verbosity.
  flag: -v
- description: file creation mode (user/group/other permissions) in octal.
  flag: -m
inlets:
  1st:
  - type: symbol
    description: directory to be created.
  - type: verbose <float>
    description: set verbosity on or off.
  - type: creationmode <octal>
    description: restrict permissions of the to-be-created file.
outlets:
  1st:
  - type: symbol
    description: created directory on success.
  2nd:
  - type: bang
    description: when there's an error creating the directory.
draft: false
---
This ensures that a given directory exists by creating it.

parent directories are created as needed.

it is not an error, if the requested directory already exists (and is a directory).
