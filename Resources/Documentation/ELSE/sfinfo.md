---
title: sfinfo
description: query sound file information

categories:
- object

pdcategory: ELSE, Audio I/O, File Management

inlets:
  1st:
  - type: read <symbol>
    description: load sound file
  - type: channels
    description: query number of channels

outlets:
  1st:
    - type: anything
      description: sound file info

draft: false
---

[sfinfo] supports all files that [sfload] and [play.file~] support and can be used to query file sample information but only channels so far as this object is still very experimental).
