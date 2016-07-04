#ifndef SdRaw_h
#define SdRaw_h

#include "Arduino.h"
#include "PixelLight.h"

#define blockSize 512
class SdRaw{
  public :
    // Ports
    int PIN_CS = PINB2;      // chip select
    int PIN_MOSI = PINB3;    // master out slave in
    int PIN_MISO = PINB4;    // master in slave out
    int PIN_CLOCK = PINB5;   // clock

    byte spi_err;
    byte vBlock[blockSize];
    byte vBuffer[16];
    byte numFrameBytes=blockSize;
    PixelLight *pix;
    SdRaw();
    byte spi_cmd(volatile char data);
    void spi_initialize(void);
    byte sdc_cmd(byte commandIndex, long arg);
    byte sdc_initialize(void);
    void sdc_clearVector(void);
    long sdc_totalNbrBlocks(void);
    void sdc_readRegister(byte sentCommand);
    void sdc_writeBlock(long blockIndex);
    void sdc_readBlock(long blockIndex);
    void sdc_printVectorContent(void);
    
    unsigned long currBlock;
    unsigned int currBlockOffset;
    byte readByte(unsigned long blockIndex,unsigned int offset);
    void resetBlock();
    byte getNextByte();
    unsigned long getNextLong();
    
    void moveToRelativePointer(unsigned long blockIndex,unsigned int offset);
    
    void loadFrame(unsigned long blockIndex);
    void loadNextFrame(unsigned long blockIndex);
    
    void sdc_writer(int cmd);
  private :
    byte clr;
};

#endif
