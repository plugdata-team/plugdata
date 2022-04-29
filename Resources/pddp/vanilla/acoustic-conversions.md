convert acoustical units

|object |Description|
|:---|---:|
|[mtof](../mtof) |convert MIDI pitch to frequency in hertz|
|[ftom](../ftom) |convert frequency in hertz to MIDI pitch|
|[dbtorms](../dbtorms) |convert db to rms|
|[rmstodb](../rmstodb) |convert rms to db|
|[powtodb](../powtodb) |convert power to db|
|[dbtopow](../dbtopow) |convert db to power|

The mtof object transposes a midi value into a frequency in Hertz, so that "69" goes to "440". You can specify microtonal pitches as in "69.5", which is a quarter tone (or 50 cents) higher than 69 (so 0.01 = 1 cent). Ftom does the reverse. A frequency of zero Hertz is given a MIDI value of -1500 (strictly speaking, it is negative infinity.)

The dbtorms and rmstodb objects convert from decibels to linear ("RMS") amplitude, so that 100 dB corresponds to an "RMS" of 1 Zero amplitude (strictly speaking, minus infinity dB) is clipped to zero dB, and zero dB, which should correspond to 0.0001 in "RMS", is instead rounded down to zero.

Finally, dbtopow and powtodb convert decibels to and from power units, equal to the square of the "RMS" amplitude.