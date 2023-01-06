---
title: send~, s~
description: send signal to one or more receive~ objects.
categories:
- object
aliases:
- s~
see_also:
- throw~
- send
- receive~
- tabsend~
pdcategory: General Audio Manipulation
last_update: '0.33'
inlets:
  1st:
  - type: signal
    description: signal to send to matching receive~ object(s).
arguments:
- type: symbol
  description: send symbol name 
  default: empty symbol
draft: false
---
A send~ object copies its input to a local buffer which all receive~ objects of the same name read from. They may be in different windows or even different patches. Any number of receives may be associated with one send~ but it is an error to have two send~s of the same name. Receive~ takes "set" messages to switch between send~s.

Send~/Receive~ only work for the default block size (64