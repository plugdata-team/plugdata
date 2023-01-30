---
title: echo.rev~

description: echo reverb

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: number of stages from 4 to 20
  type: float
  default: 8
- description: room size (0-1)
  type: float
  default: 0

inlets:
  1st:
  - type: signal
    description: echo/reverb input
  2nd:
  - type: float
    description: room size from 0 to 1

outlets:
  1st:
  - type: signal
    description: echo/reverb output

draft: false
---

[echo.rev~] is an echo/reverb abstraction. It only contains feedforward lines and can be used to produce early reflections in a reverb algorithm. But it can also be used on its own.

