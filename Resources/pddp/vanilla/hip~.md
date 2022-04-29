---
title: hip~
description: one-pole high pass filter.
categories:
- object
see_also:
- lop~
- bp~
- vcf~
- bob~
- biquad~
- slop~
- cpole~
- fexpr~
pdcategory: Audio Filters
last_update: '0.44'
inlets:
  1st:
  - type: signal
    description: audio signal to be filtered.
  - type: clear
    description: clear filter's memory.
  2nd:
  - type: float
    description: rolloff frequency.	
outlets:
  1st:
  - type: signal
    description: filtered signal. 
arguments:
  - type: float
    description: rolloff frequency in Hz (default 0).
draft: false
---
hip~ is a one-pole high pass filter with a specified cutoff frequency. Left (audio) inlet is the incoming audio signal. Right (control) inlet sets cutoff frequency.

COMPATIBILITY NOTE: in Pd versions before 0.44, the high-frequency output gain was incorrectly greater than one (usually only slightly so, but noticeably if the cutoff frequency was more than 1/4 the Nyquist frequency). This problem was fixed INCORRECTLY in pd 0.44-0 though 0.44-2, and is now hopefully fixed since Pd 0.44-3. To get the old (0.43 and earlier) behavior, set "compatibility" to 0.43 in Pd's command line or by a message.