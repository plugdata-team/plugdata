---
title: pd
description: define a subwindow (subpatch)
categories:
- object
aliases:
- page
see_also: 
- inlet
- inlet~
- outlet
- outlet~
- namecanvas
pdcategory: Subwindows
last_update: '0.52'
inlets:
  'n: depends on inlet/outlet objects':
outlets:
  'n: depends on inlet/outlet objects':	
arguments:
  - type: symbol
    description: sets the subpatch name that you can use to send messages to (see 'dynamic patching' in 'pd-messages' file.)
draft: false
---
By typing "pd" into an object box, you create a subpatch. An optional argument sets the subpatch name.
