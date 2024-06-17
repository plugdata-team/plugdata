---
title: sendmidi

description: helper to send MIDI send messages to [pd~]

categories:
 - object
 
pdcategory: ELSE, MIDI

outlets:
  1st:
  - type: anything
    description: MIDI messages to send to the [pd~] object

draft: false
---

[sendmidi] is a helper abstraction to send MIDI messages to the [pd~] object, which cannot listen to the connected MIDI devices in Pd.
