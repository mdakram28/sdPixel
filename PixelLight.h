#ifndef PixelLight_h
#define PixelLight_h

#include "Arduino.h"

class PixelCode;

class PixelLight
{
  public :
    bool* frame = 0;
    byte* nextFrame = 0;
    PixelCode *code;
    bool printEnable = true;
    unsigned int frameSize;
    unsigned long clipFrameSize;
    unsigned long clipRowWidth;
    byte rowWidth;
    void sendBit(bool bitVal);
    void sendByte(byte b);
    void ledSetup();
    void show();
    void sendOn();
    void sendOff();
    void displayFrame();
    void execute(byte func);
    void copyInNextFrame();
    void clearFrame();
    void clearNextFrame();
    void set_nfb(int i,bool data);
    bool get_nfb(int i);
    
    void rotateRight();
    void rotateLeft();
};

#endif
