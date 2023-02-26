---
title: borax
description: reports note on/off info
categories:
 - object
pdcategory: cyclone, MIDI
arguments:
inlets:
  1st:
  - type: float
    description: midi note number
  2nd:
  - type: float
    description: note velocity
  3rd:
  - type: bang
    description: resets by sending note-offs and erasing events' memory
outlets:
  1st:
  - type: float
    description: note event number from report
  2nd:
  - type: float
    description: voice number from report (when more notes are reported)
  3rd:
  - type: float
    description: number of voices (notes currently On)
  4th:
  - type: float
    description: pitch of note from report
  5th:
  - type: float
    description: velocity of note from report (0 means Note-Off)
  6th:
  - type: float
    description: event number from note duration report
  7th:
  - type: float
    description: note duration (in ms) report
  8th:
  - type: float
    description: event number from delta-time report
  nth: #9th doesn't work
  - type: float
    description: delta-time (time difference in ms) between Note-Ons

draft: false
---

[borax] sends detailed MIDI Note information.

