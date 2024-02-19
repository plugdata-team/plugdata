---
title: autofade2.mc~

description: automatic fade with indepent in/out times

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
- type: float
    description: fade time out ms
    default: 10
methods:
  - type: fadein <float>
    description: fade in time in ms
  - type: fadeout <float>
    description: fade out time in ms
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

[autofade2.mc~] is an automatic fade in/out for multichanne inputs. It responds to a gate control and uses internal lookup tables just like [fader~]. A gate on happens when the last input value was zero or less and the incoming value isn't. A gate off happens when the last value was greater than zero value and the incoming value isn't! The maximum gain depends on the gate on level.
