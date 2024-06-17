---
title: pix_video
description: open a camera and get input
categories:
  - object
pdcategory: Gem, Graphics

methods:
    - name: colorspace <symbol>
    description: decodes the current film into the specified colorspace, if supported by the backend.
  - name: device <number>
    description: the number of the input device
  - name: device <symbol>
    description: the file path to the input device
  - name: dimen <float> <float>
    description: set various dimensions for the image
  - name: enumerate
    description: list all devices to the console
  - name: driver <float>
    description: switch between different drivers, e.g. v4l, ieee1394, etc.
  - name: driver <symbol>
    description: switch between different drivers, e.g. v4l, v4l2, dv...
  - name: dialog
    description: open a dialog to configure the input device (depending on driver / Operating System)
  - name: enumProps
    description: interact with driver/device properties
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
  2nd:
    - type: list
      description: info

draft: false
---
