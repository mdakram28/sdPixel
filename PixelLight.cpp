#include "Arduino.h"
#include "PixelLight.h"
#include "SdRaw.h"

#define PIXELS 96*11 

#define PIXEL_PORT  PORTB  // Port of the pin the pixels are connected to
#define PIXEL_DDR   DDRB   // Port of the pin the pixels are connected to
#define PIXEL_BIT   4      // Bit of the pin the pixels are connected to

#define T1H  900    // Width of a 1 bit in ns
#define T1L  600    // Width of a 1 bit in ns

#define T0H  400    // Width of a 0 bit in ns
#define T0L  900    // Width of a 0 bit in ns

#define RES 6000 

#define NS_PER_SEC (1000000000L)
#define CYCLES_PER_SEC (F_CPU)

#define NS_PER_CYCLE ( NS_PER_SEC / CYCLES_PER_SEC )

#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )

void PixelLight::sendBit( bool bitVal ) {
    if (  bitVal ) {
		asm volatile (
			"sbi %[port], %[bit] \n\t"				// Set the output bit
			".rept %[onCycles] \n\t"                                // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			"cbi %[port], %[bit] \n\t"                              // Clear the output bit
			".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			::
			[port]		"I" (_SFR_IO_ADDR(PIXEL_PORT)),
			[bit]		"I" (PIXEL_BIT),
			[onCycles]	"I" (NS_TO_CYCLES(T1H) - 2),		// 1-bit width less overhead  for the actual bit setting, note that this delay could be longer and everything would still work
			[offCycles] 	"I" (NS_TO_CYCLES(T1L) - 2)
		);
    } else {
		asm volatile (
			"sbi %[port], %[bit] \n\t"				// Set the output bit
			".rept %[onCycles] \n\t"				// Now timing actually matters. The 0-bit must be long enough to be detected but not too long or it will be a 1-bit
			"nop \n\t"                                              // Execute NOPs to delay exactly the specified number of cycles
			".endr \n\t"
			"cbi %[port], %[bit] \n\t"                              // Clear the output bit
			".rept %[offCycles] \n\t"                               // Execute NOPs to delay exactly the specified number of cycles
			"nop \n\t"
			".endr \n\t"
			::
			[port]		"I" (_SFR_IO_ADDR(PIXEL_PORT)),
			[bit]		"I" (PIXEL_BIT),
			[onCycles]	"I" (NS_TO_CYCLES(T0H) - 2),
			[offCycles]	"I" (NS_TO_CYCLES(T0L) - 2)
		);
    }  
}

void PixelLight::copyInNextFrame(){
  if(nextFrame==0){
    nextFrame = new byte[frameSize/8];
  }
  for(int i=0;i<frameSize;i++){
    set_nfb(i,frame[i]);
  }
}

void PixelLight::set_nfb(int i,bool data){
  nextFrame[i/8] = (nextFrame[i/8]&(1<<(i%8))) | (data<<(i%8));
}

bool PixelLight::get_nfb(int i){
  return (nextFrame[i/8]>>(i%8))&1;
}

void PixelLight::clearFrame(){
  if(frame==0){
    frame = new bool[frameSize];
    return;
  }
  for(int i=0;i<frameSize;i++){
    frame[i] = 0;
  }
}

void PixelLight::clearNextFrame(){
  if(nextFrame==0){
    nextFrame = new byte[frameSize/8];
    return;
  }
  for(int i=0;i<frameSize/8;i++){
    frame[i] = 0;
  }
}

//send green , red , blue
void PixelLight::sendByte( unsigned char byte ) {
    for( unsigned char bit = 0 ; bit < 8 ; bit++ ) {
      sendBit( bitRead( byte , 7 ) );
      byte <<= 1;
    }           
}

void PixelLight::sendOn(){
  //green
  sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);
  //red
  sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);
  //blue
  sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);sendBit(1);
}

void PixelLight::sendOff(){
  //green
  sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);
  //red
  sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);
  //blue
  sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);sendBit(0);
}

void PixelLight::ledSetup() {
  bitSet( PIXEL_DDR , PIXEL_BIT );
}

void PixelLight::show() {
	_delay_us( (RES / 1000UL) + 1);				// Round up since the delay must be _at_least_ this long (too short might not work, too long not a problem)
}

// f(r,c) = r*rowWidth+col;

void PixelLight::displayFrame(){
  
  byte rows = frameSize/rowWidth;
  int r,c;
  
//  for(c = rowWidth-1;c>=0;){
//    for(r=0;r<rows;r++){
//      if(frame[r*rowWidth+c]){
//        sendOn();
//      }else{
//        sendOff();
//      }
//    }
//    c--;
//    for(r=rows-1;r>=0;r--){
//      if(frame[r*rowWidth+c]){
//        sendOn();
//      }else{
//        sendOff();
//      }
//    }
//    c--;
//  }
  
//  for(int i=frameSize-1;i>=0;i--){
//    if(frame[frameSize-i-1]){
//      Serial.print(" # ");
//    }else{
//      Serial.print("   ");
//    }
//    if((frameSize-i)%rowWidth==0){
//      Serial.println();
//    }
//    if(frame[i]){
//      sendOn();
//    }else{
//      sendOff();
//    }
//  }
  if(printEnable){
    for(int i=0;i<frameSize;i++){
      if(frame[i]){
        Serial.print(" #");
      }else{
        Serial.print("  ");
      }
      if((i+1)%rowWidth==0){
        Serial.println();
      }
    }
  }
//  for(int i=0;i<numFrameBytes;i++){
//    Serial.println(frameBuffer[i],BIN);
//  }
}
void PixelLight::execute(byte func){
}

void PixelLight::rotateRight(){
  byte rows = frameSize/rowWidth;
  unsigned int numFrameBytes = ceil(frameSize/8.0);
  for(int i=frameSize-1;i>0;i--){
    frame[i] = frame[i-1];
  }
  for(int i=0;i<rows;i++){
    frame[i*rowWidth] = get_nfb(i*rowWidth + rowWidth-1);
  }
  bool carry = 0;
  for(int i=0;i<numFrameBytes;i++){
    bool c2 = (nextFrame[i]&128)>>7;
    nextFrame[i] = (nextFrame[i]<<1)|carry;
    carry = c2;
  }
  nextFrame[numFrameBytes] &= 255>>(7-(frameSize-1)%8);
}

void PixelLight::rotateLeft(){
  byte rows = frameSize/rowWidth;
  unsigned int numFrameBytes = ceil(frameSize/8.0);
  for(int i=0;i<frameSize-1;i++){
    frame[i] = frame[i+1];
  }
  for(int i=0;i<rows;i++){
    frame[i*rowWidth + rowWidth-1] = get_nfb(i*rowWidth);
  }
  bool carry = 0;
  for(int i=numFrameBytes-1;i>=0;i--){
    bool c2 = nextFrame[i]&1;
    nextFrame[i] = (nextFrame[i]>>1)|(carry<<7);
    carry = c2;
  }
  //nextFrame[numFrameBytes] &= 255>>(7-(frameSize-1)%8);
}

