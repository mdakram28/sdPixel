#include "Arduino.h"
#include "SdRaw.h"

#define blockSize 512

#define GO_IDLE_STATE 0x00     // resets the SD card
#define SEND_CSD 0x09          // sends card-specific data
#define SEND_CID 0x0A          // sends card identification 
#define READ_SINGLE_BLOCK 0x11 // reads a block at byte address 
#define WRITE_BLOCK 0x18       // writes a block at byte address
#define SEND_OP_COND 0x29      // starts card initialization
#define APP_CMD 0x37           // prefix for application command 

SdRaw::SdRaw(){
  // Set ports
  // Data in
  DDRB &= ~(1<<PIN_MISO);
  // Data out
  DDRB |=  (1<<PIN_CLOCK);
  DDRB |=  (1<<PIN_CS);
  DDRB |=  (1<<PIN_MOSI); 
  // Initialize SPI and SDC 
  spi_err=0;        // reset SPI error
  spi_initialize(); // initialize SPI port
  sdc_initialize(); // Initialize SD Card
}

byte SdRaw::spi_cmd(volatile char data) {
  spi_err = 0; // reset spi error
  SPDR = data; // start the transmission by loading the output byte into the spi data register
  int i = 0;
  while (!(SPSR & (1<<SPIF))) {
    i++;
    if (i >= 0xFF) {
      spi_err = 1;
      return(0x00);
    }
  }
  // returned value
  return(SPDR); 
}

void SdRaw::spi_initialize(void) {
  SPCR = (1<<SPE) | (1<<MSTR); // spi enabled, master mode
  clr = SPSR; // dummy read registers to clear previous results
  clr = SPDR;
}

// print vector of data
void SdRaw::sdc_printVectorContent(void) {
  for (int i=0; i<blockSize; i++) {
    Serial.print("0x");
    if (vBlock[i] <= 0x0F) Serial.print("0");
    Serial.print(vBlock[i], HEX);
    Serial.print(" ");
    // append crlf to each line of 16 bytes
    if (((i+1) % 16) == 0) Serial.println(); 
  }
  Serial.println();
}

byte SdRaw::sdc_cmd(byte commandIndex, long arg) {
  PORTB &= ~(1<<PIN_CS);   // assert chip select for the card
  spi_cmd(0xFF);           // dummy byte
  commandIndex |= 0x40;    // command token OR'ed with 0x40 
  spi_cmd(commandIndex);   // send command
  for (int i=3; i>=0; i--) {
    spi_cmd(arg>>(i*8));   // send argument in little endian form (MSB first)
  }
  spi_cmd(0x95);           // checksum valid for GO_IDLE_STATE, not needed thereafter, so we can hardcode this value
  spi_cmd(0xFF);           // dummy byte gives card time to process
  byte res = spi_cmd(0xFF);
  return (res);  // query return value from card
}

byte SdRaw::sdc_initialize(void) {
  // set slow clock: 1/128 base frequency (125Khz in this case)
  SPCR |=  (1<<SPR1) | (1<<SPR0); // set slow clock: 1/128 base frequency (125Khz in this case)
  SPSR &= ~(1<<SPI2X);            // No doubled clock frequency
  // wake up SD card
  PORTB |=  (1<<PIN_CS);          // deasserts card for warmup
  PORTB |=  (1<<PIN_MOSI);        // set MOSI high
  for(byte i=0; i<10; i++) {
    spi_cmd(0xFF);                // send 10 times 8 pulses for a warmup (74 minimum)
  }
  // set idle mode
  byte retries=0;
  PORTB &= ~(1<<PIN_CS);          // assert chip select for the card
  while(sdc_cmd(GO_IDLE_STATE, 0) != 0x01) { // while SD card is not in iddle state
    retries++;
    if (retries >= 0xFF) {
      return(NULL); // timed out!
    }
    delay(5);
  }
  // at this stage, the card is in idle mode and ready for start up
  retries = 0;
  sdc_cmd(APP_CMD, 0); // startup sequence for SD cards 55/41
  while (sdc_cmd(SEND_OP_COND, 0) != 0x00) {
    retries++;
    if (retries >= 0xFF) {
      return(NULL); // timed out!
    }
    sdc_cmd(APP_CMD, 0); 
  }
  // set fast clock, 1/4 CPU clock frequency (4Mhz in this case)
  SPCR &= ~((1<<SPR1) | (1<<SPR0)); // Clock Frequency: f_OSC / 4 
  SPSR |=  (1<<SPI2X);              // Doubled Clock Frequency: f_OSC / 2 
  return (0x01); // returned value (success)
}

void SdRaw::sdc_clearVector(void) {
  for (int i=0; i<blockSize; i++) {
    vBlock[i] = 0;
  }
}

long SdRaw::sdc_totalNbrBlocks(void) {
  sdc_readRegister(SEND_CSD);
  // compute size
  long C_Size = ((vBuffer[0x08] & 0xC0) >> 6) | ((vBuffer[0x07] & 0xFF) << 2) | ((vBuffer[0x06] & 0x03) << 10);
  long C_Mult = ((vBuffer[0x08] & 0x80) >> 7) | ((vBuffer[0x08] & 0x03) << 2);
  return ((C_Size+1) << (C_Mult+2)); 
}

void SdRaw::sdc_readRegister(byte sentCommand) {
  byte retries=0x00;
  byte res=sdc_cmd(sentCommand, 0); 
  while(res != 0x00) { 
    delay(1);
    retries++;
    if (retries >= 0xFF) return; // timed out!
    res=spi_cmd(0xFF); // retry
  }  
  // wait for data token
  while (spi_cmd(0xFF) != 0xFE); 
  // read data
  for (int i=0; i<16; i++) {
    vBuffer[i] = spi_cmd(0xFF);
  }
  // read CRC (lost results in blue sky)
  spi_cmd(0xFF); // LSB
  spi_cmd(0xFF); // MSB
}

void SdRaw::sdc_writeBlock(long blockIndex) {
  byte retries=0;
  while(sdc_cmd(WRITE_BLOCK, blockIndex * blockSize) != 0x00) { 
    delay(1);
    retries++;
    if (retries >= 0xFF) return; // timed out!
  }
  spi_cmd(0xFF); // dummy byte (at least one)
  // send data packet (includes data token, data block and CRC)
  // data token
  spi_cmd(0xFE);
  // copy block data
  for (int i=0; i<blockSize; i++) {
    spi_cmd(vBlock[i]); 
  }
  // write CRC (lost results in blue sky)
  spi_cmd(0xFF); // LSB
  spi_cmd(0xFF); // MSB
  // wait until write is finished
  while (spi_cmd(0xFF) != 0xFF) delay(1); // kind of NOP
}

void SdRaw::sdc_readBlock(long blockIndex) {
  byte retries = 0x00;
  byte res = sdc_cmd(READ_SINGLE_BLOCK,  (blockIndex * blockSize));
  while(res != 0x00) { 
    delay(1);
    retries++;
    if (retries >= 0xFF) return; // timed out!
    res=spi_cmd(0xFF); // retry
  }
  // read data packet (includes data token, data block and CRC)
  // read data token
  while (spi_cmd(0xFF) != 0xFE); 
  // read data block
  for (int i=0; i<blockSize; i++) {
    vBlock[i] = spi_cmd(0xFF); // read data
  }
  // read CRC (lost results in blue sky)
  spi_cmd(0xFF); // LSB
  spi_cmd(0xFF); // MSB
}

byte SdRaw::readByte(unsigned long blockIndex,unsigned int offset){
  if(currBlock!=blockIndex){
    currBlock = blockIndex;
    sdc_readBlock(currBlock);
  }
  return vBlock[offset];
}

void SdRaw::resetBlock(){
  sdc_readBlock(currBlock);
  currBlockOffset = 0;
}

byte SdRaw::getNextByte(){
  if(currBlockOffset>=blockSize){
    currBlock++;
    sdc_readBlock(currBlock);
    currBlockOffset = 0;
  }
  return vBlock[currBlockOffset++];
}

unsigned long SdRaw::getNextLong(){
  unsigned long ret = ((unsigned long)getNextByte())<<24; 
  ret |= ((unsigned long)getNextByte())<<16; 
  ret |= ((unsigned long)getNextByte())<<8; 
  ret |= ((unsigned long)getNextByte());
  return ret;
}

void SdRaw::moveToRelativePointer(unsigned long blockIndex,unsigned int offset){
  blockIndex += offset/blockSize;
  offset = offset%blockSize;
  if(currBlock!= blockIndex){
    currBlock = blockIndex;
    resetBlock();
  }
  currBlockOffset = offset;
}

void SdRaw::loadFrame(unsigned long blockIndex){
  byte retries = 0x00;
  byte res = sdc_cmd(READ_SINGLE_BLOCK,  (blockIndex * blockSize));
  while(res != 0x00) {
    delay(1);
    retries++;
    if (retries >= 0xFF) return; // timed out!
    res=spi_cmd(0xFF); // retry
  }
  // read data packet (includes data token, data block and CRC)
  // read data token
  while (spi_cmd(0xFF) != 0xFE); 
  // read data block
  numFrameBytes = ceil(pix->frameSize/8.0);
  int b,c=0;
  if(pix->frame!=0){
    delete [] pix->frame;
  }
  pix->frame = new bool [numFrameBytes*8];
  for (int i=0; i<numFrameBytes; i++) {
    b = spi_cmd(0xFF); // read data
    pix->frame[c++] = b&1;
    pix->frame[c++] = (b&2)>>1;
    pix->frame[c++] = (b&4)>>2;
    pix->frame[c++] = (b&8)>>3;
    pix->frame[c++] = (b&16)>>4;
    pix->frame[c++] = (b&32)>>5;
    pix->frame[c++] = (b&64)>>6;
    pix->frame[c++] = (b&128)>>7;
  }
  for(int i=numFrameBytes;i<blockSize;i++){
    spi_cmd(0xFF);
  }
  // read CRC (lost results in blue sky)
  spi_cmd(0xFF); // LSB
  spi_cmd(0xFF); // MSB
}

void SdRaw::loadNextFrame(unsigned long blockIndex){
  byte retries = 0x00;
  byte res = sdc_cmd(READ_SINGLE_BLOCK,  (blockIndex * blockSize));
  while(res != 0x00) {
    delay(1);
    retries++;
    if (retries >= 0xFF) return; // timed out!
    res=spi_cmd(0xFF); // retry
  }
  // read data packet (includes data token, data block and CRC)
  // read data token
  while (spi_cmd(0xFF) != 0xFE); 
  // read data block
  numFrameBytes = ceil(pix->frameSize/8.0);
  if(pix->nextFrame!=0){
    delete [] pix->nextFrame;
  }
  pix->nextFrame = new byte[numFrameBytes];
  for (int i=0; i<numFrameBytes; i++) {
    pix->nextFrame[i] = spi_cmd(0xFF);
  }
  for(int i=numFrameBytes;i<blockSize;i++){
    spi_cmd(0xFF);
  }
  // read CRC (lost results in blue sky)
  spi_cmd(0xFF); // LSB
  spi_cmd(0xFF); // MSB
}

#define CMD_readPrint       5
#define CMD_blockRead       1
#define CMD_changeBuffer    2
#define CMD_writeBuffer     3
#define CMD_verify          4
#define CMD_clearBlock      6

void SdRaw::sdc_writer(int cmd){
  int sz;
  switch(cmd){
    case CMD_blockRead :
      sdc_readBlock(Serial.parseInt());
    break;
    case CMD_changeBuffer :
      sz = Serial.parseInt();
      for(int i=0;i<sz;i++){
        vBlock[i] = (byte)Serial.parseInt();
        Serial.print(vBlock[i]);
        Serial.print(" ");
      }
    break;
    case CMD_writeBuffer :
      sdc_writeBlock(Serial.parseInt());
    break;
    case CMD_verify :
      sz = Serial.parseInt();
      for(int i=0;i<sz;i++){
        Serial.print((int)vBlock[i],DEC);
        Serial.print(' ');
      }
      Serial.println();
    break;
    case CMD_clearBlock :
      for(int i=0;i<blockSize;i++){
        vBlock[i] = 0;
      }
    break;
    case CMD_readPrint :
      sdc_readBlock(Serial.parseInt());
      for(int i=0;i<blockSize;i++){
        Serial.print(vBlock[i],DEC);
        Serial.print(' ');
      }
      Serial.println();
    break;
  }
}
