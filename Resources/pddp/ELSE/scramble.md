---
title: scramble

description: Scramble messages

categories:
- object

pdcategory:

arguments: (none)

inlets:
  1st:
  - type: anything
    description: a message to be scrambled

outlets:
  1st:
  - type: anything
    description: the scrambled message
  2nd:
  - type: list
    description: the scrambled indices

draft: false
---

[scramble] scrambles an input message. The right outlet sends the scrambled indexes (from 0).
