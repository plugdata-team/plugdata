---
title: next

description: detect separation of messages

categories:
 - object

pdcategory: cyclone, Data Management

arguments: (none)

inlets:
  1st:
  - type: anything
    description: any messages to check if it's part of a same logical event

outlets:
  1st:
  - type: bang
    description: if the current message is not part of a logical event
  2nd:
  - type: bang
    description: if the current message is part of a logical event

draft: true
---

[next] sends a bang to the left outlet when an incoming message is not part of the same "logival event" as the previous message, and a bang out its right outlet otherwise. An "event" is a single message, a mouse click, key press, MIDI event.
