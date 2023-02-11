---
title: loadmess
description: send a message when the patch is loaded
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: anything
  description: message to be sent
  default: bang
inlets:
  1st:
  - type: bang
    description: outputs its typed message
outlets:
  1st:
  - type: anything
    description: the loaded message (when the patch is loaded or when banged/clicked)

flags:
  - name: @defer <0/1>
    description: output is deferred (@defer 0 will be sent before @defer 1), default: 0

methods:
  - type: set <anything>
    description: set followed by any message sets the message held by loadmess without any output

---

[loadmess] outputs a message automatically when the patch is loaded, or when the patch is an abstraction of another patch that is opened.
