#ifndef PixelCode_h
#define PixelCode_h

#include "Arduino.h"
#include "SdRaw.h"

class PixelCodeFunction
{
  public :
    PixelCodeFunction();
    void customFunction(byte func);
};

class PixelCode
{
  public :
    bool debug = false;
    unsigned int vars[50];
    byte varBase = 0;
    unsigned long fileBlock;
    SdRaw *sd;
    PixelCodeFunction* customFunc;
    unsigned int callStack[8];
    byte callPointer = 0;
    byte stackPointer = 0;
    byte varCount = 0;
    PixelCode();
    void runFile(unsigned long blockIndex);
    void exec();
    byte getNextCodeByte();
    unsigned int getNextCodeInt();
    
    unsigned int getNextVar();
    unsigned int pullVar();
};

#endif
