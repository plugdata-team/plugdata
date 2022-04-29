---
title: block~
description: set block size for DSP
categories:
- object
see_also: 
- fft~
- bang~
- switch~
pdcategory: Subwindows
last_update: '0.43'
inlets:
  1st:
  - type: set <list>
    description: set argument values (size, overlap, up/downsampling).
arguments:
- type: float
  description: set block size (default 64).
- type: float
  description: set overlap for FFT (default 1).
- type: float
  description: up/down-sampling factor (default 1). 

draft: false
---
The block~ and switch~ objects set the block size, overlap, and up/down-sampling ratio for the patch window. (The overlap and resampling ratio are relative to the super-patch.)

Switch~, in addition, allows you to switch DSP on and off for the DSP on the patch window. All subwindows are also switched. (If a subwindow of a switched window is also switched, both switches must be on for the subwindow's audio DSP to run. Pd's global DSP must also be on.)

You may have at most one block~/switch~ object in any window.

A switch~ or block~ object without arguments does not reblock audio computation - in other words, block size and sample rate are as in the parent patch.

Switch~ also takes a "bang" message that causes one block of DSP to be computed if it's switched off. This is useful for pre-computing waveforms, window functions or also for video processing.

Pd's default block size is 64 samples. The inlet~ and outlet~ objects reblock signals to adjust for differences between parent and subpatch, but only power-of-two adjustments are possible. So for "normal" audio computations, all blocks should also be power-of-two in size. HOWEVER, if you have no inlet~ or outlet~ you may specify any other block size. This is intended for later use in video processing.