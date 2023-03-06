---
title: grain.sampler~

description: granular sampler

categories:
- object

pdcategory: ELSE, Effects, Buffers

arguments:

inlets:
  1st:
  - type: bang
    description: plays an event of pitch clouds
  2nd:
  - type: float
    description: number of grains in cloud event (minimum 1, maximum 256)

outlets:
  1st:
  - type: signal
    description: left channel output
  2nd:
  - type: signal
    description: right channel output

flags:
  - name: -n <f>
    description:
  - name: -t <symbol>
    description: table name
  - name: -dur <f>
    description:
  - name: -size <f, f>
    description:
  - name: -transp <f, f>
    description:
  - name: -pan <f, f>
    description:
  - name: -sync
    description:
  - name: -env <any>
    description:
  - name: -amps <f, f>
    description:
  - name: -pos <f, f>
    description:
  - name: -autotune <f>
    description:
  - name: -scale <list>
    description:
  - name: -rev <float>
    description:

methods:
  - type: n <float>
    description: number of grains in cloud event (minimum 1, maximum 256)
  - type: sync <f>
    description: non-0 sets to synchronous mode (default 0)
  - type: dur <f>
    description: sets cloud event duration in ms (default 500)
  - type: set <symbol>
    description: sets table name with loaded sample
  - type: sr <float>
    description: sets sample rate for reading the buffer (default Pd's)
  - type: size <f, f>
    description: sets min/max grain sizes in ms (default 50 to 450)
  - type: transp <f, f>
    description: sets min/max transposition in cents (default -1200 to 1200)
  - type: pos <f, f>
    description: sets min/max grain position (default 0 to 1)
  - type: pan <f, f>
    description: sets left and right pan boundaries (default -1 to 1)
  - type: amp <f, f>
    description: min/max amplitude values (default 0.1 to 1)
  - type: rev <float>
    description: sets probability in % of grain being reversed (default 0)
  - type: env <any>
    description: envelope type (sin, hann, tri, gauss) or function list
  - type: autotune <f>
    description: non-0 autotunes to a given scale (default 0)
  - type: scale <list>
    description: scale to autotune to in cents (default equal temperament)


draft: false
---

[grain.sampler~] is a sample based granulator that generates clouds of grains.

