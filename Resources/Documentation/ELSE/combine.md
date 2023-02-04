---
title: combine

description: combine data received close together

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: initial time in ms
  type: float
  default: 0

inlets:
  1st:
  - type: anything
    description: message to be combined with another input close in time
  2nd:
  - type: float
    description: time interval for combining items to a list

outlets:
  1st:
  - type: anything
    description: combined (or not) messages

draft: false
---

[combine] collects messages (with one or more elements) into a single list if they come within a certain given amount of time, but not if greater than it. A "0" time is possible if messages are sent in a sequence such as from a [loop] object. Each item or list is appended to the previous stored items. A negative time value will not combine anything.
