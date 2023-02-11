---
title: file splitname
description: separates directory an file components
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
- file splitext
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
    description: file component if no directory component is found
draft: false
---
This object perform common string operations on filenames. no checks are performed verifying the validity/existence of any path-component.

This object perform common string operations on filenames. no checks are performed verifying the validity/existence of any path-component.

'splitname' separates the directory component from the file component of the path, and outputs a list with both on the 1st outlet.

If no directory component can be found, the path is sent to the 2nd outlet.

A trailing slash is removed.

The filename is the non-empty string after the last path-separator

NOTE: the 'file split*' objects will always output (lists of) symbols, even if the component looks like a number. on one hand this makes it harder to [route], but on the other hand this will keep zero-padded numbers intact (think "2020/01/01/0042.wav").
