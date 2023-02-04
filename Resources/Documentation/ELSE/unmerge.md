---
title: unmerge

description: split messages

categories:
 - object
 
pdcategory: ELSE, Data Management

arguments:
  - type: float
    description: number of outlets from 2-512 (default 2), there's an extra outlet for the extra elements
  - type: float
    description: elements number size per outlet (default 1)

flags:
  - name: -trim
    description: trims the selector

inlets:
  1st:
  - type: anything
    description: a message whose elements get unmerged
  2nd:
  - type: size <float>
    description: number of elements in a group
    
outlets:
  1st:
  - type: anything
    description: the extra elements of the message
  nth:
  - type: anything
    description: the bits of the unmerged message



draft: false
---

[unmerge] separates the elements of a message into groups of any size (default 1). Each group is sent out a separate outlet, extra elements are sent to an extra rightmost outlet.
