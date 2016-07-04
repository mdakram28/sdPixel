#include "PixelCode.h"
#include "Arduino.h"
#include "SdRaw.h"

PixelCode::PixelCode(){}

void PixelCode::runFile(unsigned long blockIndex){
	fileBlock = blockIndex;
	sd->currBlock = blockIndex;
	sd->resetBlock();
	exec();
}

#define CMD_exec           0x01
#define CMD_return         0x02
#define CMD_jump           0x05
#define CMD_setVar         0x06
#define CMD_ifEqual        0x07
#define CMD_ifNotEqual     0x08
#define CMD_addVars        0x09
#define CMD_incrementVar   0x0A
#define CMD_ifZero         0x0B
#define CMD_jumpIfZero     0x0C
#define CMD_jumpIfNotZero  0x0D
#define CMD_initVar        0x0E
#define CMD_push           0x0F
#define CMD_popInVar       0x10
#define CMD_clearStack     0x11

#define CMD_add            0x12
#define CMD_sub            0x13
#define CMD_mul            0x14
#define CMD_div            0x15
#define CMD_mod            0x16
#define CMD_equ            0x17
#define CMD_neq            0x18
#define CMD_gth            0x19
#define CMD_lth            0x1A
#define CMD_gte            0x1B
#define CMD_lte            0x1C
#define CMD_and            0x1D
#define CMD_orr            0x1E

void PixelCode::exec(){
	unsigned int var1 , var2;
	byte var3,var4;
	stackPointer -= varCount;
	while(true){
		byte cmd = getNextCodeByte();
                if(debug){
                  for(int i=varBase;i<varBase+varCount;i++){
                    Serial.print(vars[i]);
                    Serial.print("\t");
                  }
                  Serial.println();
                  Serial.print("stack :: ");
                  for(int i=varBase+varCount;i<varBase+varCount+stackPointer;i++){
                    Serial.print(vars[i]);
                    Serial.print("\t");
                  }
                  Serial.println();
                  Serial.print("COMMAND :: ");
                  Serial.print(cmd);
                  Serial.print('\t');
                  Serial.println(callStack[callPointer]);
                  Serial.print("vars  :: ");
                  delay(1000);
                }
		switch(cmd){
			case CMD_exec:
				var1 = getNextCodeInt();//address
				var2 = getNextCodeByte();//param Count
				var3 = varBase;
                varBase += varCount + stackPointer - var2;
				callPointer++;
				callStack[callPointer] = var1;
				sd->moveToRelativePointer(fileBlock,callStack[callPointer]);
				var4 = varCount;
				varCount = var2;
				exec();
				varCount = var4;
				varBase = var3;
				sd->moveToRelativePointer(fileBlock,callStack[callPointer]);
			break;
			case CMD_return:
				callPointer--;
				return;
			break;
			case CMD_push:
				vars[varBase+varCount+stackPointer] = getNextVar();
				stackPointer++;
			break;
			case CMD_popInVar:
				stackPointer--;
				var3 = getNextCodeByte(); //veriable address
				vars[varBase+var3] = vars[varBase+varCount+stackPointer];
                                if(debug){
                                  Serial.print("Popping in variable :: ");
                                  Serial.println(var3);
                                }
			break;
			case CMD_jump:
				callStack[callPointer] = getNextCodeInt();
				sd->moveToRelativePointer(fileBlock,callStack[callPointer]);
			break;
			case CMD_clearStack:
				stackPointer = 0;
			break;
			case CMD_setVar:
				var3 = getNextCodeByte();//var num
				vars[varBase+var3] = getNextVar();//val
				if(var3>varCount)varCount = var3;
			break;
			case CMD_initVar:
				var3 = getNextCodeByte();//var num
				if(var3+1>varCount)varCount = var3+1;
                                if(debug){
                                  Serial.print("Added variable :: ");
                                Serial.println(var3);
                                }
			break;
			case CMD_jumpIfNotZero:
				var1 = pullVar();
				var2 = getNextCodeInt();
				if(var1!=0){
					callStack[callPointer] = var2;
					sd->moveToRelativePointer(fileBlock,callStack[callPointer]);
				}
			break;
			case CMD_jumpIfZero:
				var1 = pullVar();
				var2 = getNextCodeInt();
				if(var1==0){
					callStack[callPointer] = var2;
					sd->moveToRelativePointer(fileBlock,callStack[callPointer]);
				}
			break;


			case CMD_add:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3]+vars[var3 -1];
			break;
			case CMD_sub:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]-vars[var3];
			break;
			case CMD_mul:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]*vars[var3];
			break;
			case CMD_div:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]/vars[var3];
			break;
			case CMD_mod:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]%vars[var3];
			break;
			case CMD_equ:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]==vars[var3];
			break;
			case CMD_neq:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]!=vars[var3];
			break;
			case CMD_gth:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]>vars[var3];
			break;
			case CMD_lth:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]<vars[var3];
			break;
			case CMD_gte:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]>=vars[var3];
			break;
			case CMD_lte:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]<=vars[var3];
			break;
			case CMD_and:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]&vars[var3];
			break;
			case CMD_orr:
				stackPointer--;
				var3 = varBase+varCount+stackPointer;
				vars[var3 -1] = vars[var3 -1]|vars[var3];
			break;

			default:
				customFunc->customFunction(cmd);
		}
	}
	
}


unsigned int PixelCode::pullVar(){
	return vars[varBase+varCount+ --stackPointer];
}

byte PixelCode::getNextCodeByte(){
	callStack[callPointer]++;
	return sd->getNextByte();
}

unsigned int PixelCode::getNextCodeInt(){
	int ret = ((int)getNextCodeByte())<<8;
	ret |= (int)getNextCodeByte();
	return ret;
}


unsigned int PixelCode::getNextVar(){
	byte type = getNextCodeByte();
	if(type==1){
		return vars[varBase+getNextCodeInt()];
	}else{
		return getNextCodeInt();
	}
}
