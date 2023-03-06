---
title: pv

description: private (local) variable

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: symbol
  description: variable name
  default:
- type: anything
  description: initial stored message
  default:

inlets:
  1st:
  - type: anything
    description: message to store and share by other [pv] objects
  - type: bang
    description: output the stored message

outlets:
  1st:
  - type: anything
    description: the stored message

methods:
- type: status
  description: posts information in the Pd window about related [pv] objects in the patch

draft: true
---

[pv] is similar to pd's [value], but stores any message as a variable name, which can be shared within a patch or its subpatch if it has another [pv] object with the same variable name. It won't share with other patches!
