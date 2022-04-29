acoustic conversions for audio signals

|object |Description|
|:---|---:|
|[mtof~](../mtof~) |convert MIDI pitch to frequency in hertz|
|[ftom~](../ftom~) |convert frequency in hertz to MIDI pitch|
|[dbtorms~](../dbtorms~) |convert db to rms|
|[rmstodb~](../rmstodb~) |convert rms to db|
|[powtodb~](../powtodb~) |convert power to db|
|[dbtopow~](../dbtopow~) |convert db to power|

These objects convert MIDI pitch to frequency and back, and dB to and from RMS and power. They take audio signals as input and output (and work sample by sample.) Since they call library math functions, they may be much more expensive than other workaday tilde objects such as *~ and osc~, depending on your hardware and math library.

Boundary conditions are handled "reasonably". 100 db is assigned an RMS of 1, and dbtorms~ and dbtopow~ output true zero for 0 dB and less.