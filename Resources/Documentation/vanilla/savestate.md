---
title: savestate
description: save and restore run-time state from within an abstraction
categories:
- object
pdcategory: vanilla, UI, File Management
last_update: '0.49'
inlets:
  1st:
  - type: list
    description: one or more list when the parent patch gets saved
outlets:
  1st:
  - type: list
    description: one or more list when the parent patch is opened
  2nd:
  - type: bang
    description: when the parent patch is saved
draft: false
---
The savestate object is used inside abstractions to save their state as they are used in a calling (parent) patch. When the parent patch (such as this one, which calls the "savestate-example" abstraction) is saved, the included savestate object sends a 'bang' message out its right outlet, with which the abstraction may respond by presenting one or more 'list' messages back to the 'savestate' object. These lists are saved as part of the calling patch. If the calling patch is reopened later, the lists are sent out the left outlet of the savestate object. The abstraction can then use them to restore its state.

Note that abstractions within 'clone' objects are not handled!

The abstraction may itself be modified at will without disturbing the saved states of its copies in any calling patches, as long as the usage of the saved and restored lists is kept compatible.

The saved messages are output when the object is recreated, before any outside connections are made and possibly before other parts of a saved patch have been restored. You can use a "loadbang" object to send messages to objects elsewhere in the owning patch at load time once the entire patch is loaded.

Multiple savestate objects aren't differentiated, so they all receive all lists sent to any one of them and output them.

Hint: 'text' objects can be saved/restored using 'text tolist' and 'text fromlist'.
