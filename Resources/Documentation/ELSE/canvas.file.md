---
title: canvas.file

description: search for file

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth level
  default: 0

inlets:
  1st:
  - type: symbol
    description: file name to search

outlets:
  1st:
  - type: symbol
    description: path and filename when found
  2nd:
  - type: symbol
    description: input file when failing search

draft: false
---

[canvas.file] tries to locate the file in using the patch's directory or Pd's search-paths. If it finds it, it returns the absolute path in the left outlet. If it fails, the input name is output via the right outlet. The depth level sets the directory according to the parent patch and so on.
