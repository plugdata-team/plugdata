---
title: catch~
description: catch signal from one or more throw~ objects.
categories:
- object
see_also:
- send~
pdcategory: General Audio Manipulation
last_update: '0.33'
outlets:
  1st:
  - type: signal
    description: signal from matching throw~ object(s).
arguments:
- type: symbol
  description: catch~ name symbol 
  default: empty symbol
draft: false
---
Any number of throw~ objects can add into one catch~ object (but two catch~ objects cannot share the same name.