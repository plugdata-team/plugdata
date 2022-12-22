---
title: pd~
description: run a pd sub-process
categories:
- object
see_also:
- stdout
pdcategory: Extra
last_update: '0.42'
inlets:
  1st:
  - type: pd~ start <anything>
    description: start a new sub-process. This message takes startup flags and needs a pd file to open.
  - type: pd~ stop
    description: stops the pd sub-process.
  - type: anything
    description: messages besides "pd~" are sent to corresponding objects in the sub-process with the same name as the first element in the message.
  - type: signal
    description: signal input if there's a corresponding adc~ input.
  "'n' total number of 'n' signal inlets is given as a creation argument.":
  - type: signal
    description: signal input if there's a corresponding adc~ input.
outlets:
  1st:
  - type: anything
    description: messages from the sub-process sent via "stdout" objects.
  "'n' total number of 'n' signal outlets is given as a creation argument.":
  - type: signal
    description: signal output if there's a corresponding dac~ output.
flags:
- flag: -ninsig <float>
  description: sets number of input audio channels 
  default: 2
- flag: -noutsig <float>
  description: sets number of output audio channels 