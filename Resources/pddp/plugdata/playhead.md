---
title: playhead

description: playhead

categories:
- object

arguments:

inlets:

outlets:
  1st:
  - type: float
    description: playing (1 if daw is playing, 0 otherwise)
  2nd:
  - type: float
    description: recording (1 if recording, 0 otherwise)
  3rd:
  - type: float
    description: looping (1 if looping, 0 otherwise)
  4th:
  - type: float
    description: edittime
  5th:
  - type: float
    description: framerate
  6th:
  - type: float
    description: bpm
  7th:
  - type: float
    description: lastbar
  8th:
  - type: float
    description: timesig
  nth:  #9th doesn't work
  - type: float
    description: position (in beats)

---
