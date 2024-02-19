---
title: autofade.mc~

description: automatic fade in/out

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
  - type: symbol
    description: fade types <quartic, sin, sqrt, lin, hann, lin, hannsin, linsin>
    default: quartic
  - type: float
    description: fade time in ms
    default: 10

methods:
  - type: fade <float>
    description: fade time in ms
  - type: anything
    description: fade types <quartic, sin, sqrt, hann, lin, hannsin, linsin>

inlets:
  1st:
  - type: signals
    description: input signals to be faded in/out
  2nd:
  - type: float/signal
    description: gate on/off

outlets:
  1st:
  - type: signals
    description: autofaded signals

draft: false
---

[autofade.mc~] is an automatic fade in/out for multichanne inputs. It responds to a gate control and uses internal lookup tables just like [fader~]. A gate on happens when the last input value was zero or less and the incoming value isn't. A gate off happens when the last value was greater than zero value and the incoming value isn't! The maximum gain depends on the gate on level.
