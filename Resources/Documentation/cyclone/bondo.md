---
title: bondo
description: sync a group of messages
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: sets the number of inlets/outlets
  default: 2
- type: float
  description: sets the sync time
  default: 0
- type: symbol
  description: ''n' sets outlets to send messages with more than 1 element
inlets:
  nth:
  - type: bang
    description: outputs last stored values
  - type: anything
    description: one element messages go to the corresponding outlet. If there are more than 1 element, they're parsed among the inlets unless the "n" argument is used
outlets:
  nth:
  - type: anything
    description: messages from corresponding input

draft: false
---

[bondo] synchronizes and outputs messages when any inlet gets a message. It can wait a time interval for input messages (see [pd sync time]) or send them automatically.

