---
title: rec
description: multi-track message recorder

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: number of tracks
  default: 1 
- type: symbol
  description: .txt file to open
  default:

inlets:
  1st:
  - type: play <list>
    description: plays all tracks or tracks from the given list
  - type: speed <float>
    description: sets playback speed in %
  - type: record <list>
    description: records all tracks or tracks from the given list
  - type: stop <list>
    description: stops (rec/play) all tracks or tracks from the given list
  - type: mute <list>
    description: mutes all tracks or tracks from the given list
  - type: unmute <list>
    description: unmutes all tracks or tracks from the given list
  - type: clear <list>
    description: clears all tracks or tracks from the given list
  - type: open <symbol>
    description: opens a text file
  - type: save <symbol>
    description: saves to a text file
  2nd:
  - type: anything
    description: any message to be recorded in that inlet/track


outlets:
  1st:
  - type: anything
    description: the reversed message/list

draft: false
---

[reverse] reverses messages/lists.
