---
title: snake~
description: combine or split multichannel signals
categories:
- object
pdcategory: vanilla, Mixing and Routing
last_update: '0.54'
see_also:
- inlet~
- clone
- throw~
- send~

flags:
- name: in
  description: sets the combine mode
- name: out
  description: sets the split mode

arguments:
- description: number of channels
  default: 2
  type: float

inlets:
  nth:
  - type: float/signal
    description: mono signals to merge (default) or mc signal to split (split mode)

outlets:
  nth:
  - type: signals
    description: multichannel signal (default) or split mono signals
draft: false
---
Sound engineers call multi-channel audio cables "snakes." The [snake~] object can be invoked as [snake~ in] to create a muiltichannel signal from a set of single-channel ones, or [snake~ out] to extract the channels in a multi-channel signal. Typing just [snake~] defaults to [snake~ in]. In either case, a numeric argument sets the number of channels.
