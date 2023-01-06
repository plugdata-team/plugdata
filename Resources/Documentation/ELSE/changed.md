---
title: changed

description: Pass data when changed

categories:
 - object

pdcategory: Message Management

arguments:
- type: anything
  description: initializes first message
  default: empty symbol atom

inlets:
  1st:
  - type: anything
    description: any message to pass through
  - type: bang
    description: force the last data to be output
  2nd:
  - type: anything
    description: sets a current message inside the object

outlets:
  1st:
  - type: anything
    description: any mesesage that's different from the previous
  2nd:
  - type: anything
    description: unchanged message

draft: false
---

[changed] passes its data input through only when it changed from the last receive message or the message that was set via arguments or messages. Unlike [change] from Pd Vanilla, it accepts any kind of messages.