---
title: autofade2~

description: automatic fade-in/out

categories:
 - object

pdcategory: ELSE, Mixing and Routing, Effects

arguments:
  - type: symbol
    description: (optional) fade types <quartic, sin, sqrt, lin, hann, lin, hannsin, linsin>
    default: quartic
  - type: float
    description: fade in time in ms
    default: 0
  - type: float
    description: fade out time in ms
    default: 0
  - type: float
    description: number of channels
    default: 1

inlets:
  1st:
  - type: signal
    description: input signal to be faded in/out
  - type: anything
    description: fade types <quartic, sin, sqrt, hann, lin, hannsin, linsin>
  nth:
  - type: signal
    description: input signal to be faded in/out
  2nd:
  - type: float/signal
    description: gate; "fade in" if != 0, "fade out" otherwise

outlets:
  nth:
  - type: signal
    description: autofaded signal
  2nd: #rightmost
  - type: float
    description: a 1 or 0 is sent when the fade in/out is finished

methods:
  - type: fadein <float>
    description: fade in time in ms
  - type: fadeout <float>
    description: fade out time in ms

draft: false
---

[autofade2~] is an automatic fade in/out (with separate time for fade in/out) for multichannel inputs. It responds to a gate control and uses internal lookup tables for 7 different fading curves. A gate-on happens when the last input value was zero and the incoming value isn't. A gate-off happens when the last value was a non-0 value and the incoming value is 0 The maximum gain depends on the gate on level.
