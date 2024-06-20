
#define slave_id 5
byte RxData[8];
 byte TxData[50];
int indx ;

static uint16_t Holding_Registers_Database[50]={
		0000,  1111,  2222,  3333,  4444,  5555,  6666,  7777,  8888,  9999,   // 0-9   40001-40010
		12345, 15432, 15535, 10234, 19876, 13579, 10293, 19827, 13456, 14567,  // 10-19 40011-40020
		21345, 22345, 24567, 25678, 26789, 24680, 20394, 29384, 26937, 27654,  // 20-29 40021-40030
		31245, 31456, 34567, 35678, 36789, 37890, 30948, 34958, 35867, 36092,  // 30-39 40031-40040
		45678, 46789, 47890, 41235, 42356, 43567, 40596, 49586, 48765, 41029,  // 40-49 40041-40050
};

uint8_t readHoldingRegs (void)
{
	uint16_t startAddr = ((RxData[2]<<8)|RxData[3]);  // start Register Address

	uint16_t numRegs = ((RxData[4]<<8)|RxData[5]);   // number to registers master has requested
	
	uint16_t endAddr = startAddr+numRegs-1;  // end Register

  indx = 0;
	TxData[indx++] = RxData[0];  //slave id 
  TxData[indx++] = RxData[1];  //fun code
  TxData[indx++] = numRegs*2;   //byte count
for (int i=0; i<numRegs; i++)   // Load the actual data into TxData buffer
 {
   TxData[indx++] = (Holding_Registers_Database[startAddr]>>8)&0xFF;  // extract the higher byte
   TxData[indx++] = (Holding_Registers_Database[startAddr])&0xFF;   // extract the lower byte
   startAddr++;  // increment the register address
 }
return 1;
}

void setup() {
  // put your setup code here, to run once:
pinMode(3, OUTPUT);
pinMode(4, OUTPUT);
Serial.begin(9600);
 
}

void loop()
 { 
  // put your main code here, to run repeatedly:
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  if(Serial.available() )
  {
  Serial.readBytes(RxData, 8);
 // Serial.print(Serial.readString()); 
  for(int i=0; i<8; i++)
  {
    Serial.println(RxData[i]);
  }
  }

  if (RxData[0] == slave_id)
   {
     switch (RxData[1])
     {
      case 3: if (readHoldingRegs() == 1)
              { 
                digitalWrite(3, HIGH);
                digitalWrite(4, HIGH);
                Serial.write(TxData, indx);
              }
              break;
     }
   }
}




