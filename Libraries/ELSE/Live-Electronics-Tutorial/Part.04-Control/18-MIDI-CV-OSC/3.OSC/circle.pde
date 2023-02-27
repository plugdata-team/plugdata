
import oscP5.*; // you need these adding for OSC/Network connection.
import netP5.*; // install them if you haven't already

OscP5 oscP5;
NetAddress myRemoteLocation;

void setup(){
  size(800, 800); // window size
  frameRate(25);
  oscP5 = new OscP5(this, 12001); // init oscP5 listening on port 12001
}

float ratio = 100; // init variable

void draw(){ // draw circle
  background(0);  
  circle(width/2, height/2, ratio);
}

void oscEvent(OscMessage OSCin){ // receive OSC message
  if(OSCin.checkAddrPattern("/ratio")) // check if address is "ratio"
      ratio = OSCin.get(0).floatValue(); // set ratio value in variable
}
