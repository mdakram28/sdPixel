#include "SdRaw.h"
#include "PixelCode.h"
#include "PixelLight.h"

SdRaw sd = SdRaw();
PixelCode code = PixelCode();
 
#define func_digitalWrite 100
#define func_delay 101
#define func_timeStamp 102

#define func_playClip 203
#define func_loadFrame 204
#define func_setRowWidth 205
#define func_loadNextFrame 206
#define func_setFrameSize 207
#define func_rotateRight 208
#define func_display 209
#define func_clearFrame 210
#define func_clearNextFrame 211
#define func_rotateLeft 212
#define func_setPrintEnable 213
#define func_clipConfig 214

long time;
unsigned long displayStartTime=0;
unsigned int displayTargetTime=0;

PixelLight pix = PixelLight();

PixelCodeFunction::PixelCodeFunction(){}
void PixelCodeFunction::customFunction(byte func){
  unsigned int var1 , var2;
  long endTime = 0;
  switch(func){
    case func_rotateRight :
      displayTargetTime = code.pullVar();
      for(int i=0;i<pix.rowWidth;i++){
        pix.rotateRight();
        while(millis()-displayStartTime<displayTargetTime);
        if(displayTargetTime>0){
          pix.displayFrame();
        }
        displayStartTime = millis();
        Serial.println(i);
      }
    break;
    case func_rotateLeft:
      displayTargetTime = code.pullVar();
      for(int i=0;i<pix.rowWidth;i++){
        pix.rotateLeft();
        while(millis()-displayStartTime<displayTargetTime);
        pix.displayFrame();
        displayStartTime = millis();
        Serial.println(i);
      }
    break;
    case func_setPrintEnable :
      pix.printEnable = code.pullVar();
    break;
    case func_clearFrame :
      pix.clearFrame();
    break;
    case func_clearNextFrame :
      pix.clearNextFrame();
    break;
    case func_loadFrame :
      var1 = code.pullVar();
      var2 = code.pullVar();
      sd.loadFrame(code.fileBlock + var1+var2);
    break;
    case func_loadNextFrame :
      var1 = code.pullVar();
      var2 = code.pullVar();
      sd.loadNextFrame(code.fileBlock + var1+var2);
    break;
    case func_setFrameSize :
      pix.frameSize = code.pullVar();
    break;
    case func_display :
      pix.displayFrame();
    break;
    case func_setRowWidth :
      var1 = code.pullVar();
      Serial.print("Set row width :: ");
      Serial.println(var1);
      pix.rowWidth = var1;
    break;
    case func_clipConfig :
      pix.clipFrameSize = code.pullVar();
      pix.clipRowWidth = code.pullVar();
    break;
    case func_playClip :
      var1 = code.getNextCodeByte(); //frame count
      pix.frameSize = code.getNextCodeInt();  //frameSize
      for(int i=0;i<var1;i++){
        sd.loadFrame(code.fileBlock + code.getNextCodeInt());
        //Serial.print("delay : ");
        //Serial.println();
        while(millis()-displayStartTime<displayTargetTime);
        pix.displayFrame();
        displayStartTime = millis();
        displayTargetTime = code.getNextCodeInt();
        Serial.println(i);
      }
    break;
    case func_digitalWrite :
      var1 = code.pullVar();
      var2 = code.pullVar();
      digitalWrite(var1,var2);
      Serial.print("digital write : ");
      Serial.print(var1);
      Serial.print('\t');
      Serial.print(var2);
      Serial.println();
    break;
    case func_delay :
      var1 = code.pullVar();
//      Serial.print("delay .... ");
//      Serial.println(var1);
      //delay(1000);
      delay(var1);
    break;
    case func_timeStamp :
      endTime = millis();
      Serial.println(endTime-time);
      time = endTime;
    break;
    default :
      pix.execute(func);
    break;
  }
}

PixelCodeFunction customFunc = PixelCodeFunction();

void setup(){
  Serial.begin(115200);
  pix.ledSetup();
  code.sd = &sd;
  code.customFunc = &customFunc;
  sd.pix = &pix;
  pix.code = &code;
}

void loop(){
  int devStart = millis();
  while((millis()-devStart)<500){
    if(Serial.available()){
      while(true){
        int cmd = Serial.parseInt();
        if(cmd==100)break;
        sd.sdc_writer(cmd);
      }
    }
  }
  code.runFile(100);
}
