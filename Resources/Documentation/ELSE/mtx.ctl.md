---
title: mtx.ctl

description: matrix control GUI

categories:
- object

pdcategory: ELSE, UI, Mixing and Routing

arguments:

inlets:
  1st:
  - type: list
    description: sets and outputs column, row and status

outlets:
  1st:
  - type: list
    description: column, row, status


flags:
 - name: -n <f, f>
   description: number of columns and rows
 - name: -size <float>
   description: cell size in pixels
 - name: -embed
   description: embed flag
   
methods: 
  - type: set <list>
    description: sets column, row and status (no output)
  - type: get <list>
    description: outputs value of row/column
  - type: clear
    description: clears active cells
  - type: flush
    description: flushes active cell values
  - type: export
    description: exports contents as an array
  - type: import
    description: imports contents as an array
  - type: rows <float>
    description: sets number of rows
  - type: columns <float>
    description: sets number of columns
  - type: size <float>
    description: sets cell size in pixels
  - type: embed <float>
    description: non-0 save internal contents with the object

draft: false
---

[mtx.ctl] provides a user matrix interface control. The cells in the grid can be either on or off. The output is a list with 3 floats that specify: column, row and status.
