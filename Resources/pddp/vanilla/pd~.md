---
title: pd~
description: run a pd sub-process
categories:
- object
see_also:
- stdout
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
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
  description: sets number of input audio channels (default 2).
- flag: -noutsig <float>
  description: sets number of output audio channels (default 2).
- flag: -sr <float>
  description: sets sample rate of subprocess (default pd's current).
- flag: -fifo <float>
  description: sets number of blocks for round-trip (default 5).
- flag: -pddir <symbol>
  description: sets Pd's directory (needed if different than default).
- flag: -scheddir <symbol>
  description: sets scheduler's directory (also needed if different).
draft: false
---
The pd~ object starts and manages a Pd sub-process that can communicate with the super-process via audio channels and/or Pd messages. In this way you can take advantage of multi-core CPUs, and/or use Pd features from within Max (if you're using the Max version of pd~).

Messages with "pd~" selector control the sub-process. "pd~ start" takes as arguments any startup flags you wish to send the sub-process. For example, specify "-nogui" to stop the sub-process's GUI from appearing. You don't have to specify the number of channels in and out, since that's set by creation arguments below. Audio config arguments arguments (-audiobuf, -audiodev, etc.) are ignored.

The first outlet reports messages the sub-process sends us via "stdout" objects. Any other outlets are signals corresponding to "dac~" objects in the sub-process.

ATTENTION: DSP must be running in the super-process for the sub-process to run. This is because its clock is slaved to audio I/O it gets from the super-process!