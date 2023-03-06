---
title: osc.route
description: route OSC messages

categories:
 - object

pdcategory: ELSE, Networking

arguments:
  - type: anything
    description: list of OSC addresses to route to

inlets:
  1st:
  - type: anything
    description: messages to be routed

outlets:
  nth:
  - type: anything
    description: routed messages that correspond to an argument
  2nd:
  - type: anything
    description: unmatched message

draft: false
---

[osc.route] routes OSC messages received from [osc.receive]. It follows the same logic as Vanilla's [route] where arguments set the number of outlets and an extra is created for no matches. The difference is that it manages OSC addresses instead, specified with the "/" separator. Note how the '/hz' address matches input messages that start with '/hz' even if it has further sub addresses.

