---
title: tosymbol

description: convert anything to symbol

categories:
 - object

pdcategory: cyclone, Converters

arguments: (none)

inlets: 
  1st:
  - type: bang
    description: outputs a bang
  - type: anything
    description: message to convert to symbol

outlets:
  1st:
  - type: symbol
    description: converted symbol message
  
  flags:
  - name: @separator <symbol>
    description: sets the separator character
    default: "space"

methods:
  - type: separator <symbol>
    description: sets the separator character
    default: "space"

draft: true
---

[tosymbol] converts any kind of message to a symbol message (maximum of 2048 characters). Start by sending it a float with the number box below and see how it converts to a symbol message.
