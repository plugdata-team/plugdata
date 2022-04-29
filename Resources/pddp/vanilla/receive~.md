---
title: receive~
description: receive signal from a send~ object.
categories:
- object
aliases:
- r~
see_also:
- throw~
- send
- send~
- tabsend~
pdcategory: General Audio Manipulation
last_update: '0.33'
inlets:
  1st:
  - type: set <name>
    description: set receive name.
outlets:
  1st:
  - type: signal
    description: outputs signal from a matching send~ object.
arguments:
- type: symbol
  description: receive name symbol (default empty symbol).
draft: false
---
A send~ object copies its input to a local buffer which all receive~ objects of the same name read from. They may be in different windows or even different patches. Any number of receives may be associated with one send~ but it is an error to have two send~s of the same name. Receive~ takes "set" messages to switch between send~s.

Send~/Receive~ only work for the default block size (64);
for FFT applications see also: tabsend~.

