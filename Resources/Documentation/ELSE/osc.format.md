---
title: osc.format
description: format OSC messages

categories:
 - object

pdcategory: ELSE, Networking

arguments:

inlets:
  1st:
  - type: anything
    description: OSC message to be formatted

outlets:
  1st:
  - type: list
    description: formatted OSC message
draft: false
---

[osc.format] is similar to Vanilla's [oscformat]. This is still way too simple and experimental. It is used in the [osc.send] abstraction and can be used for edge and lower level cases. It takes anything messages or lists whose first element is an address that start with "/". The type of data for each element in the message is either a float or a string only depending on the atom type, so there are only these options (so far). You can't send bundles either. Use the original Vanilla [oscformat] or mrpeach's packOSC object for more complete objects.

