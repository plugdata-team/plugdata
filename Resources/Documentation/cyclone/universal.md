---
title: universal

description: sends messages to all objects of the same type

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: non-0 also sends to subpathces
  default: 0

inlets: 
  1st:
  - type: anything
    description: sends any message to an object type

outlets:

methods:
 - type: send <anything>
   description: sends any message to an object type
   default: 

draft: true
---

[universal] sends messages to all instances of the same object in a patch and/or in a subpatch window.
