---
title: autofade~

description: Automatic fade-in/out

categories:
 - object

pdcategory: General

arguments:
  1st:
  - type: symbol
    description: (optional) fade types <quartic, sin, sqrt, lin, hann, lin, hannsin, linsin>
    default: quartic
  2nd:
  - type: float
    description: fade in/out time in ms
    default: 0
  3rd:
  - type: float
    description: number of channels
    default: 1

inlets:
  1st:
  - type: float/signal
    description: - gate; "fade in" if != 0, "fade out" otherwise
  - type: anything
    description: fade types <quartic, sin, sqrt, hann, lin, hannsin, linsin>
  - type: fade <float>
    description: fade in/out time in ms
  Nth:
  - type: signal
    description: input signal to be faded in/out

outlets:
  Nth:
  - type: signal
    description: autofaded signal
  Nth+1:
  - type: float
    description: a 1 or 0 is sent when the fade in/out is finished

draft: false
---

[autofade~] is an automatic fade in/out for multichannel inputs. It responds to a gate control and uses internal lookup tables for 7 different fading curves. A gate-on happens when the last input value was zero and the incoming value isn't. A gate-off happens when the last value was a non-zero value and the incoming value is 0! The maximum gain depends on the gate on level.
