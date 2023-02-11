---
title: coll
description: store/edit message collections
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: symbol
  description: name or filename to load (same name colls share contents)
- type: float
  description: non-0 prevents from searching for a file
  default: 0
inlets:
  1st:
  - type: bang
    description: outputs next address and its data
  - type: float/symbol
    description: an "int float" or "symbol" address: outputs stored data
  - type: list
    description: 1st element is an int address to store remaining data to
outlets:
  1st:
  - type: anything
    description: the stored message at a given address
  2nd:
  - type: float/symbol
    description: integer or symbol address of the stored message
  3rd:
  - type: bang
    description: when finishing a read operation
  4th:
  - type: bang
    description: when finishing a dump operation

flags:
  - name: @embed
    description: <1> save contents with the patch, <0> don't save (default)
  - name: @threaded
    description: <1> threaded processing (default), <0> unthreaded

methods:
  - type: assoc <symbol, int>
    description: associates a symbol alias to an int address
  - type: clear
    description: deletes all addresses (the complete data collection) from [coll]
  - type: deassoc <symbol, int>
    description: deassociates the symbol alias of an int address
  - type: delete <int/symbol>
    description: deletes the address (if an int, higher addresses are decremented by 1)
  - type: dump
    description: dumps all values (1st outlet), addresses (2nd) and bangs (4th) when done
  - type: end
    description: sets the pointer to the last entry/address
  - type: embed <float>
    description: <1> saves contents with the patch, <0> doesn't
  - type: filetype <symbol>
    description: sets file type to read/write (no symbol sets no extension: default)
  - type: flags <float, float>
    description: first flag is the embed flag, second is unused (just use 'embed' instead)
  - type: goto <int/symbol>
    description: sets an address to go to (only sets the pointer, no output)
  - type: insert <int, anything>
    description: an int address and any message to insert, increments equal/greater addresses
  - type: insert2 <int, anything>
    description: same as insert, but only increments if necessary to include the new address
  - type: length
    description: sends number of stored values to the left outlet
  - type: max <int>
    description: sends max value of all <nth> elements (starts at 1, default) on left outlet
  - type: merge <int/symbol, any>
    description: sets address and any message to be appended to the stored message
  - type: min <float>
    description: sends min value of all <nth> elements (starts at 1, default) on left outlet
  - type: next
    description: same as bang: outputs next address and its data
  - type: nth <int/symbol, int>
    description: address and nth element to be output (starts at 1)
  - type: nstore <int, symbol, any>
    description: or <symbol, int, any>: stores any data to an int address with a symbol alias
  - type: nsub <int/sym, int, any>
    description: the address, an element (starts at 1) and any value to substitute it to
  - type: open
    description: opens the window with the data collection (allows manual data editing)
  - type: prev
    description: outputs previous address and its data
  - type: read <symbol>
    description: opens file specified by the symbol (without a symbol, a dialog window opens)
  - type: readagain
    description: reopen last file - if no file had been open, a dialog box is shown
  - type: refer <symbol>
    description: refers to the data from another [coll] object with that symbol name
  - type: remove <int/symbol>
    description: removes address and its data without renumbering (unlike delete)
  - type: renumber <int>
    description: lists int addresses consecutively, argument sets starting value (default 0)
  - type: renumber2 <int>
    description: increments int addresses by one, starting from the given address (default 0)
  - type: separate <int>
    description: increments by 1 addresses from the given <int> and above (opening a slot)
  - type: sort <float, float>
    description: <flag> -1: ascending / 1: descending, <sort element> from 0 (-1 is address)
  - type: start
    description: sets the pointer to the first entry/address
  - type: store <int/sym, any>
    description: int or symbol address and any message to be stored at it
  - type: sub <int/sym, int, any>
    description: same as nsub, but the substituted message is sent out
  - type: subsym <symbol, symbol>
    description: substitutes an address symbol (2nd element) by the first given symbol
  - type: swap <int/sym, int/sym>
    description: swaps data between two addresses
  - type: wclose
    description: closes the data window containing the collection
  - type: write <symbol>
    description: saves file specified by the symbol (without a symbol, a dialog window opens)
  - type: writeagain
    description: resaves last file - if no file had been saved, a dialog box is shown

---

[coll] stores/edits any messages at given addresses (an integer or a symbol). If an input list starts with an int, it stores the other element(s) at that int address.

