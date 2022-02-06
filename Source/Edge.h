/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

class Connection;
class Box;
class Edge : public TextButton
{
 public:
  Box* box;

  Edge(Box* parent, bool isInput);


  void paint(Graphics&) override;
  void resized() override;

  void mouseMove(const MouseEvent& e) override;
  void mouseDrag(const MouseEvent& e) override;

  void createConnection();

  bool hasConnection();

  Rectangle<int> getCanvasBounds();

  int edgeIdx;
  bool isInput;
  bool isSignal;

 private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Edge)
};
