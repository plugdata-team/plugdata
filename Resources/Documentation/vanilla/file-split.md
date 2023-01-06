---
title: file split
description: '''split'' path into components'
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
- file delete
- file join
- file splitext
- file splitname
inlets:
  1st:
  - type: symbol
    description: file or directory name to split.
outlets:
  1st:
  - type: list
    description: list of split components.
  2nd:
  - type: symbol/bang
    description: '''/'' if inputs ends with ''/'' or bang otherwise.'
draft: false
---
This objects perform common string operations on filenames. no checks are performed verifying the validity/existence of any path-component.

NOTE: 'file split' will always output (lists of) symbols, even if the component looks like a number. on one hand this makes it harder to [route], but on the other hand this will keep zero-padded numbers intact (think "2020/01/01/0042.wav").
