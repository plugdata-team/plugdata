---
title: osc.receive
description: receive OSC messages

categories:
 - object

pdcategory: ELSE, Networking

arguments:
  - type: float
    description: port to connect to at load time

inlets:
  1st:
  - type: close
    description: close connection
  - type: connect <float>
    description: connect to a port

outlets:
  1st:
  - type: anything
    description: received OSC message

draft: false
---

[osc.receive] receives OSC messages from network connections and is an abstraction based on [osc.parse] and [netreceive].

