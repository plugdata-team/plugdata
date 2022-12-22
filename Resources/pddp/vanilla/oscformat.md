---
title: oscformat
description: OSC messages to and from Pd lists
categories:
- object
pdcategory: I/O 
last_update: 0.51.
see_also:
- oscparse
- fudiformat
- netsend
- list
arguments:
- description: list of one or more addresses
  type: list
inlets:
  1st:
  - type: format <symbol>
    description: 'characters set format types: ''b'' (blob),  ''i'' (interger),  ''f''
      (float) or ''s'' (sring).'
  - type: list
    description: list to format into a OSC packet.
  - type: set <list>
    description: set one or more adresses.
outlets:
  1st:
  - type: list
    description: converted OSC packet from lists.
draft: false
---
Oscformat makes OSC (Open Sound Control) packets (byte by byte) suitable for sending over the network via netsend (in UDP binary mode). The OSC address (the strings between the slashes) are given by the creation arguments or by "set" messages. Oscparse takes lists of numbers interpreting them as the bytes in an OSC message and outputs a list containing, first, the symbols making up the address of the OSC packet, and following that, numbers and symbols as present in the OSC message.

If a format is given (via the '-f' flag or 'format' message) oscformat interprets incoming data as integer, float, string, or 'blob'. Blobs are given as an atom count followed by that number of elements. (If an elements is a symbol, its first byte is sent). If the count is negative, the entire remaining message is included in the blob (but the OSC parser will report the actual number of elements). If the elements aren't exhausted at the end of the format string, the default (float and symbol) conversions are made for the rest.

Note: there's no way using oscparse to distinguish between floats and integers, nor to see blobs unambiguously. OSC messages may be combined in "bundles". If oscparse receives a bundle it simply parses all the messages in the bundle in the order they appear, and ignores the bundle's time tag.
