---
title: keyname
description: grab keyboard input
categories:
- object
pdcategory: Misc
last_update: '0.32'
see_also:
- key
- keyname
outlets:
  1st:
  - type: float
    description: 1 when key is pressed and 0 when released.
  2nd:
  - type: symbol
    description: key name.
draft: false
---
keyname gives the symbolic name of the key on the right outlet, with a 1 or 0 in the left outlet if it's up or down, and works with non-printing keys like shift or "F1".

Caveat -- this only works if Pd actually gets the key events which can depend on the stacking order of windows and/or the pointer location, depending on the system.
