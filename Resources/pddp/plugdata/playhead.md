---
title: playhead

description: Receive DAW playhead

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
  - type: list <f,f,f>
    description: 1 and start and end of loop if looping, zeros otherwise
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
  - type: list
    description: timesig
  nth:  #9th doesn't work
  - type: list <f,f,f>
    description: position <ppq position, time in samples, time in seconds>

---

[playhead] receives the playhead from the DAW, including tempo, current time in ms or samples, and more. Only works in plugin version!

