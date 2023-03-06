---
title: retrieve

description: retrieve data from "receives"

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: symbol
  description: receive name
  default: none

inlets: 
  1st:
  - type: bang
    description: take the data

outlets:
  1st:
  - type: anything
    description: the retrieved data

methods:
  - type: set <symbol>
    description: sets a receive name

draft: false
---

[retrieve] retrieves values from things that are connected to a "receive" symbol that matches the argument. It works with [receive], [receiver] and built-in receives in GUIs. In a sense, it uses "receive" names as a "send" as well.
