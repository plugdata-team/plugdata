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
    description: current item

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
    default: 0
  - name: -nokeep
    description: forget items on reopen
    default: 0
  - name: -savestate
    description: remember last state
    default: 0
  - type: -var <symbol>
    description: assigns a variable to the menu
    default: empty
  - type: -param <symbol>
    description: assigns a parameter to the menu
    default: empty
  - type: -mode <float>
    description: set oputput mode, 0=index, 1=item, 2=both
    default: 0

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
    description: set oputput mode, 0=index (default), 1=item, 2=both
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
[popup] is a popup menu GUI. When you click on it, you get a popup menu to choose elements from (symbols or floats). You can choose by clicking on the selected item or via up/down arrow keys and hitting 'enter'. By default, the output is the index, but can also be the element or both.
