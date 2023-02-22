---
title: del~

description: delay line

categories:
 - object

pdcategory: ELSE, Effects, Buffers

arguments:
- type: symbol
  description: "in" or "out" to define functionality
  default: "in"

inlets:

outlets:

draft: false
---

[del~] sets and writes to a delay line if created as [del~ in] (default) and reads from it (with interpolation) if created as [del~ out]. It's quite similar to [delread~]/[delread4~], but with more features.

