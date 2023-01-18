---
title: loadmess
description: Send a message when the patch is loaded
categories:
 - object
pdcategory: General
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
    description: when enabled, the output of the loadmess object is deferred (a loadmess with @defer 0 will be sent before and have priority over @defer 1) (default 0)

methods:
  - type: set <anything>
    description: set followed by any message sets the message held by loadmess without any output

---

[loadmess] outputs a message automatically when the patch is loaded, or when the patch is an abstraction of another patch that is opened.
