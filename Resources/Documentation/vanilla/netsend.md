---
title: netsend
description: send Pd messages over a network
categories:
- object
pdcategory: vanilla, Networking
last_update: '0.51'
see_also:
- netreceive
flags:
- name: -u
  description: sets UDP connection (default TCP)
- name: -b
  description: sets to binary mode (default FUDI)
inlets:
  1st:
  - type: list
    description: works like 'send'
outlets:
  1st:
  - type: float
    description: non-0 if connection is open, zero otherwise
  2nd:
  - type: anything
    description: messages sent back from netreceive objects

methods:
  - type: connect <list>
    description: sets host and port number, an additional port argument can be set for messages sent back from the receiver
  - type: disconnect
    description: close the connection
  - type: timeout <float>
    description: TCP connect timeout in ms (default 10000)
  - type: send <anything>
    description: sends messages over the network
draft: false
---
