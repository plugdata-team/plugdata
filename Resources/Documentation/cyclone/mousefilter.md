---
title: mousefilter
description: filters data with the mouse click
categories:
 - object
pdcategory: cyclone, UI
arguments:
inlets:
  1st:
  - type: anything
    description: any message to be filtered out or passed
outlets:
  1st:
  - type: anything
    description: the passed messages (if the mouse button is up)

---

[mousefilter] doesn't let messages through when the mouse button is down/clicked (but the last received message is output when the mouse button is released).

