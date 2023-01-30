---
title: receiver

description: receive messages

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: list
  description: receive names
  default:

inlets:
  1st:
  - type: symbol/list
    description: set the receive names
  - type: bang
    description: clears the receive names

outlets:
  1st:
  - type: anything
    description: any received messages (or receive names at bangs)

methods:
  - type: clear
    description: clears the receive names


draft: false
---

[receiver] is much like vanilla's [receive], but can have up to two names and has an inlet to set the receive names.
