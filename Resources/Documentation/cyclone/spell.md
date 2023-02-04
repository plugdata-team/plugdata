---
title: spell

description: convert input to ACSII values

categories:
 - object

pdcategory: cyclone, Converters

arguments:
- type: float
  description: minimum output size
  default: 0
- type: float
  description: fill character in ASCII
  default: 32 (space)

inlets: 
  1st:
  - type: anything
    description: message to convert to ASCII values

outlets:
  1st:
  - type: float
    description: the ascii representation of each input digit/character

draft: true
---

[spell] takes any message and converts each containing digit and character to UTF-8 (Unicode) values ([spell] doesn't understand non-integer float messages). The 1st argument sets the minimum output size. If the input doesn't "spell" to the minimum, the output is filled with characters (32 space character by default and specified by 2nd argument).
