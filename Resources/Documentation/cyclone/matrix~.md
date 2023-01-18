---
title: matrix~
description: Signal routing/mixing matrix
categories:
 - object
pdcategory: General
arguments:
- type: float
  description: `n` number of inputs (1 to 250, default 1)
  default: 0
- type: float
  description: `n` number of outputs (1 to 499, default 1)
  default: 0
- type: float
  description: sets to `non-binary` mode and the gain value for connections
  default: binary
inlets:
  1st:
  - type: list
    description: in binary mode: <inlet#, outlet#, on/off>. In non binary mode: <inlet#, outlet#, gain, ramp>
  - type: signal
    description: signals to route and mix
  nth:
  - type: signal
    description: signals to route and mix
outlets:
  nth:
  - type: signal
    description: routed/mixed signals from inlets
  2nd: #rightmost
  - type: list
    description: on <dump> messages, rightmost outlet dumps a list with all connections: <inlet#, outlet#, gain>

flags:
  - name: @ramp <float>
    description: only in non-binary mode (that is, if all 3 arguments are given, you can include a ramp value as an attribute) - default is 10 ms

methods:
  - type: connect <list>
    description: connects any inlet specified by the 1st value to outlet(s) specified by remaining value(s)
  - type: disconnect <list>
    description: disconnects any inlet specified by the 1st value to outlet(s) specified by the remaining value(s)
  - type: clear
    description: removes all connections
  - type: ramp <float>
    description: sets ramp (fade) time - only in non-binary mode
  - type: dump
    description: outputs current state of all connections in the rightmost outlet a list: <inlet#, outlet#, gain>
  - type: print
    description: same as dump, but prints on the terminal window
  - type: dumptarget
    description: outputs target state of all connections in the rightmost outlet a list: <inlet#, outlet#, gain>

---

[matrix~] routs signals from any inlets to one or more outlets. If more than one inlet connects to an outlet, the output is the sum of the inlets' signals.

