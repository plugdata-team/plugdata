---
title: trace
description: message tracing for debugging
categories:
- object
pdcategory: vanilla, Analysis
last_update: '0.52'
inlets:
  1st:
  - type: anything
    description: any message to be traced from this point on
  2nd:
  - type: float
    description: arms the object for the given number of messages
outlets:
  1st:
  - type: anything
    description: bypasses the input message further down the chain
draft: false
---
When 'set-tracing' is on and the object is armed,  trace prints in the Pd window the message it receives and also outputs the messages all the other objects further down in the chain send. You can control-click on the printout to select in the patch the object that caused the message. Once this is done,  trace also prints a backtrace of messages leading up to the one that has set it off.
