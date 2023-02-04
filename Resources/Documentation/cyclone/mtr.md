---
title: mtr
description: multi-track message recorder
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
  - type: float
    description: number of tracks (max 64)
    default: 1
inlets:
  1st:
  - type: anything
    description: methods
  nth:
  - type: anything
    description: any message to be recorded in that inlet/track
outlets:
  1st:
  - type: list
    description: track number & duration (when receiving the `next` message)
  nth:
  - type: anything
    description: recorded messages from the corresponding inlet/track

methods:
  - type: clear <list>
    description: in left inlet: clears all tracks. If followed track numbers, clears those track(s). In other inlets: clears corresponding track
  - type: delay <float>
    description: in left inlet: sets a delay value in ms for all tracks to start playing. In other inlets: sets a delay time to that track only
  - type: first <float>
    description: in left inlet: waits a given time in ms after a play message is received before playing back. Unlike delay, first does not alter the delta time value of the first event in a track, it just waits a certain time (in addition to the first delta time) before playing back from the beginning. In other inlets: waits only corresponding track
  - type: mute <list>
    description: in left inlet: mutes all tracks while still playing. If followed by track numbers, it mutes those track(s). In other inlets: mutes corresponding track
  - type: next <list>
    description: in left inlet: causes each track to output only the next message in its recorded sequence (the track number and the delta time of each message being output are sent out the leftmost outlet as a list). If followed track numbers, outputs the next message stored in those tracks. In other inlets: outputs the next message stored on corresponding track
  - type: play <list>
    description: in left inlet: plays recorded tracks out the corresponding outlets in the same rhythm/speed as recorded. If followed by track numbers, it plays those tracks. In other inlets: plays corresponding track
  - type: read <symbol>
    description: in left inlet: opens a previously saved file with the symbol name (or opens a dialog box if no symbol is given). In other inlets: opens a file containing only the track that corresponds to the inlet
  - type: record <list>
    description: in left inlet: begins recording all messages received in the other inlets. If followed by track numbers, it begins recording those tracks. In other inlets: begins recording corresponding track
  - type: rewind <list>
    description: in left inlet: goes to start of recorded sequence. Can be used when using the 'next' message. If [mtr] is playing or recording, a stop message should precede it. If followed by track numbers, it rewinds those tracks. In other inlets: rewinds corresponding track
  - type: stop <list>
    description: in left inlet: stops all recording/playing tracks. If followed by track numbers, it stops those tracks. In other inlets: stops corresponding track
  - type: unmute <list>
    description: in left inlet: Unmutesd all muted tracks. If followed by track numbers, it unmutes those tracks. In other inlets: unmute corresponding track
  - type: write <symbol>
    description: in left inlet: saves a file with the symbol name (or opens a dialog box if no symbol is given). In other inlets: saves corresponding track

---

[mtr] records any messages in different tracks and plays them back. Each track records what comes into its inlet and plays it back through the outlet directly below.

