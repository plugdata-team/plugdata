---
title: print
description: print messages to terminal window
categories:
- object
pdcategory: vanilla, Analysis, UI
see_also:
- print~
arguments:
- description: message to distinct one [print] from another
  type: list
flags:
- name: -n
  description: the default "print:" prefix is suppressed
inlets:
  1st:
  - type: anything
    description: any message to print into the terminal window
draft: false
---
Print prints out the messages it receives on the "terminal window" that Pd is run from. If no argument is given,  the message has a "print:" prefix. Any message as an argument is used as the prefix instead (so you can differentiate between different printouts).

You can also do command/control + click on the terminal window and the corresponding [print] object will be selected in your patch.
