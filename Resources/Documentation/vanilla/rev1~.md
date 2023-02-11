---
title: rev1~
description: simple 1-in, 1-out reverberator
categories:
- object
see_also:
- rev2~
- rev3~
pdcategory: vanilla, Effects
last_update: '0.30'
inlets:
  1st:
  - type: signal
    description: reverb input
  2nd:
  - type: float
    description: dB after 1 second
  3rd:
  - type: bang
    description: clear the reverb
outlets:
  1st:
  - type: signal
    description: reverb output
draft: false
---
This is an experimental reverberator design composed of a series of allpass filters with exponentially growing delay times. Each allpass filter has a gain of 0.7. The reverb time is adjusted by adjusting the input gains of the allpass filters. The last unit is modified so that its first two "echos" mimic those of an allpass but its loop gain depends on reverb time.

Reverb time is controlled by specifying the dB gain (100 normal) after one second, so that 100 corresponds to infinite reverb time, 70 to two seconds, 40 to one second, and 0 to 0

The "clear" button impolitely clears out all the delay lines, You may immediately resume pumping the reverberator, but the input signal should be cleanly enveloped. The output, too, must be enveloped and may not be opened until 5 msec after the "clear" message is sent.

The rev1~ module eats about 18% of my 300mHz P2 machine.
