---
title: soundfiler
description: read and write tables to soundfiles
categories:
- object
pdcategory: Arrays & Tables
last_update: '0.51'
see_also:
- array
- readsf~
- tabplay~
- tabread4~
- tabwrite~
- writesf~
inlets:
  1st:
  - type: read <list>
    description: 'sets a filename to open and optionally one or more arrays to load
      channels. `Optional flags`: -wave,  -aiff,  -caf,  -next,  -skip <float>,  -maxsize
      <float>,  -ascii,  -raw <list>.'
  - type: write <list>
    description: 'sets a filename to write and one or more arrays to specify channels.
      `Optional flags`: -wave,  -aiff,  -caf,  -next,  -big,  -little,  -skip <float>,  -nframes
      <float>,  -ascii,  -normalize,  -rate <float>.'
outlets:
  1st:
  - type: float
    description: number of samples (when reading a file).
  2nd:
  - type: list
    description: sample rate,  header size,  number of channels,  bytes per sample
      & endianness (when reading a file).
draft: false
---
The soundfiler object reads and writes floating point arrays to binary soundfiles which may contain uncompressed 2- or 3-byte integer ("pcm") or 4-byte floating point samples in wave, aiff, caf, next, or ascii text formats. The number of channels of the soundfile need not match the number of arrays given (extras are dropped and unsupplied channels are zeroed out).

The number of channels is limited to 64.

### Flags for 'read' messages:

- -wave, -aiff, -caf, -next

- -skip &lt;sample frames to skip in file&gt;

- -resize (resizes arrays to the size of the sound file)

- -maxsize &lt;maximum number of samples we can resize to&gt;

- -raw &lt;headersize&gt; &lt;channels&gt; &lt;bytespersample&gt; &lt;endianness&gt;

  - you can leave soundfiler to figure out which of the known soundfile formats the file belongs to or override all header and type information using the "-raw" flag, which causes all header and type information to be ignored. Endianness is "l" ("little") for Intel machines or "b" ("big") for older PPC Macintoshes. You can give "n" (natural) to take the byte order your machine prefers.

- -ascii - read a file containing ascii numbers

  - May be combined with -resize. Newlines in the file are ignored, non-numeric fields are replaced by zero. If multiple arrays are specified, the first elements of each array should come first in the file, followed by all the second elements and so on (interleaved).

### Flags for 'write' messages:

- -wave, -aiff, -caf, -next, -ascii

- -big, -little (sample endianness)

- -skip &lt;number of sample frames to skip in array&gt;

- -nframes &lt;maximum number to write&gt;

- -bytes &lt;2, 3, or 4&gt;

- -normalize

- -rate &lt;sample rate&gt;
