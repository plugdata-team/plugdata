---
title: popmenu

description: GUI popup menu

categories:
 - object

pdcategory: ELSE, UI

arguments:
inlets:
  1st:
  - type: float
    description: sets and outputs value
  - type: bang
    description: outputs value
  - type: set <float>
    description: sets the selected item

outlets:
  1st:
  - type: float
    description: knob's value

flags:
  - name: -load <float>
    description: sets load value, no float sets current
    default: 0
  - name: -size <float>
    description: sets size in pixels
    default: 50
  - name: -bg <list>
    description: sets background color in hexdecimal or RGB list
    default: #dfdfdf
  - name: -fg <list>
    description: sets foreground color in hexdecimal or RGB list
    default: #000000
  - name: -send <symbol>
    description: sets send symbol
    default: empty
  - name: -receive <symbol>
    description: sets receive symbol
    default: empty
  - name: -noloadbang
    description: disable output on init
    default: empty
  - name: -savestate
    description: remember last state
    default: empty
  - name: -nooutline
    description: disable outline
    default: empty

methods:
  - type: fontsize <float>
    description: sets the font size
  - type: width <float>
    description: sets the width of the menu
  - type: height <float>
    description: sets the height of the menu
  - type: clear
    description: clears all menu items
  - type: add <list>
    description: adds an item to the menu
  - type: bg <list>
    description: sets the background color
  - type: fg <list>
    description: sets the foreground color
  - type: receive <symbol>
    description: sets the receive symbol for messages
  - type: send <symbol>
    description: sets the send symbol for messages
  - type: var <symbol>
    description: assigns a variable to the menu
  - type: param <symbol>
    description: assigns a parameter to the menu
  - type: mode <float>
    description: sets the interaction mode
  - type: label <symbol>
    description: sets the menu label
  - type: outline <float>
    description: toggles the menu outline
  - type: keep <float>
    description: keeps the menu open after selection
  - type: pos <float>
    description: sets the menu position
  - type: lb <float>
    description: sets the lower bound
  - type: load <list>
    description: loads a list of menu items
  - type: savestate <float>
    description: saves the current state
  - type: loadbang <float>
    description: triggers when the object is loaded

draft: false
---
