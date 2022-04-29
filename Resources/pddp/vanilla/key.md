---
title: key
description: grab keyboard input
categories:
- object
pdcategory: Misc
last_update: '0.32'
see_also:
- keyup
- keyname
outlets:
  1st:
  - type: float
    description: key number when pressed.
draft: false
---
Key and keyup report the (system dependent) numbers of "printing" keys of the keyboard. Key outputs when the keyboard key is pressed while keyup outputs it when you release the key. Check your system's preferences for 'autorepeat' as it affects the output of these objects.

Caveat -- this only works if Pd actually gets the key events which can depend on the stacking order of windows and/or the pointer location, depending on the system.
