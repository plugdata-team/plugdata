---
title: file define
description: share file handles.
categories:
- object
pdcategory: Misc
last_update: '0.52'
see_also:
- text
- array
- list
- file
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
- file splitname
arguments:
- description: declared file-handle name.
  type: symbol
draft: false
---
Sometimes it is easier to access the same file-handle from different parts of the patch (e.g. when reading and parsing a file), rather than going backwards and forwards with a single 'file handle' object.

In this case, you can use the 'file define' object to provide a file handle that can then be accessed by multiple 'file handle' objects.
