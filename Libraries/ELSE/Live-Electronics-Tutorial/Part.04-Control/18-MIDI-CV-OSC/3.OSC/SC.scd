// GET MESSAGES FROM PURE DATA

/*
This "OSC definition gets OSC messages, its name is "\Pd" and the message is retrieved as the 'msg' argument, which we print below with 'postln'. The '/SC' is the message adress and recvPort sets to listen to port 8000.
*/

s.boot; // first boot the server

// evaluate, send message from Pd and check the post.
OSCdef(\Pd, {|msg| [msg].postln;}, '/SC', recvPort:8017)

OSCdef.freeAll // free OSC definition

/* the message is an array and we can get just item '1', which is the float sent from Pd*/

OSCdef(\Pd, {|msg| msg[1].postln;}, '/SC', recvPort:8017)

/* now lets create a sine wave which gets frequency values via a 'hz' argument */
~sine = {arg hz = 440; SinOsc.ar(hz, mul: 0.2)}.play

/* The following OSCdef expands its function to set the sine wave's 'hz' argument with the float from the message*/

(OSCdef(\Pd,{|msg| ~sine.set(\hz, msg[1]);},
	'/SC', recvPort:8017))

/* that's it, just a simple connection so you can hear something with lots of fun */

OSCdef.freeAll

~sine.free

// SEND MESSAGES TO PURE DATA

/* Sending messages is much simpler, all you need to do is creat a network address with NetAddr. Below we create one with locahost (127.0.0.1) and port '8031' */

b = NetAddr.new("127.0.0.1", 8031);

// Now we can send a message with a proper "/" address and it will be received by Pd with the [osc.receive] object listening to the same port.

b.sendMsg("/hello", "there");

