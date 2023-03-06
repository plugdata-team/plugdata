---
title: loadbanger, lb

description: multiple loadbangs

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: number of outputs
  default: 1

inlets:
  1st:
  - type: anything
    description: any message triggers bangs

outlets:
  nth:
  - type: bang
    description: bang messages at loading or when activated

flags:
  - name: -init
    description: sets loadbang to "init mode"

draft: false
---

[loadbanger] (or [lb]) sends "bangs" (from right to left) when loading the patch and also sends bangs when receiving any message or clicked on. The number of outputs is defined by the argument (1 by default). [lb] can also be useful for dynamic patching (for creating inlets/outlets) in init mode.

