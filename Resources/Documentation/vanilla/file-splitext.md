---
title: file splitext
description: separates directory+file and extension
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
- file delete
- file split
- file join
- file splitname
inlets:
  1st:
  - type: symbol
    description: file name
outlets:
  1st:
  - type: list
    description: the separated directory and file components
  2nd:
  - type: symbol
    description: path if no extension is found
draft: false
---
'splitext' separates the directory+file component from the extension of the given path, and outputs a list with both on the 1st outlet.

If no extension is found, the path is sent to the 2nd outlet.

An extension is a non-empty string after the last '.' in the filename-component of the path.

NOTE: the 'file split*' objects will always output (lists of) symbols, even if the component looks like a number. on one hand this makes it harder to [route], but on the other hand this will keep zero-padded numbers intact (think "2020/01/01/0042.wav").
