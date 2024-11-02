
#include<HardwareSerial.h>

#define RXD2 16  //esp32 Rx
#define TXD2 17  //esp32 Tx

const int DOUT_PIN = 22;
const int SCK_PIN = 23;

int gain = 128, GAIN = 1;


uint8_t data[3] = { 0 };
uint8_t filler = 0x00;
uint8_t tx_data[9];
float in_kg;

//int offset = -56693 , kg_weight = 39276, calib_weight = 1000;
float grams_fun;

///////////      shifting function
uint8_t shiftInSlow(uint8_t bitOrder)
{
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i)
    {
       digitalWrite(SCK_PIN, HIGH);  //sck high
       delayMicroseconds(1);
        if(bitOrder == LSBFIRST)
        {
            value |= digitalRead(DOUT_PIN) << i; //dout reading
            // Serial.println("lsb bits");
            // Serial.println(value);
        }
        else
        {
            value |= digitalRead(DOUT_PIN) << (7 - i);     //dout reading
            // Serial.println("msb bits");
            // Serial.println(value);
        }
        digitalWrite(SCK_PIN, LOW); //sck low
        delayMicroseconds(1);
    }
    return value;
}

///////////      read function
signed long read()
{
//	 Define structures for reading data into.

		// Pulse the clock pin 24 times to read the data.
			data[2] = shiftInSlow( MSBFIRST);
			data[1] = shiftInSlow( MSBFIRST);
			data[0] = shiftInSlow( MSBFIRST);

			// Set the channel and the gain factor for the next reading using the clock pin.
			for (unsigned int i = 0; i < GAIN; i++)
			{
				digitalWrite(SCK_PIN, HIGH);
				delayMicroseconds(1);
				digitalWrite(SCK_PIN, LOW);
				delayMicroseconds(1);
			}

			// Replicate the most significant bit to pad out a 32-bit signed integer
				if (data[2] & 0x80) 
        {
					filler = 0xFF;
				} 
        else 
        {
					filler =0; // = 0x00;
				}

				// Construct a 32-bit signed integer
			//	num = ((unsigned long)filler << 24) | ((unsigned long)data[2] << 16) | ((unsigned long)data[1] << 8) | (unsigned long)data[0];
        signed long num = ((signed long)filler << 24) | ((signed long)data[2] << 16) | ((signed long)data[1] << 8) | (signed long)data[0];
				return (num);
}

///////////      average function
long read_average(uint8_t times)
{
	long sum = 0;
	for (uint8_t i = 0; i < times; i++) 
  {
		sum += read();
		// Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
		// https://github.com/bogde/HX711/issues/73
	  //delay(4);
	}
	return sum / times;
}
// ///////////////  weight function
// float weight_fun(int update, int off, int kg_wei)
// {
  
//   float res = (-off + update)*calib_weight/kg_wei;
//   return res;
// }




void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(DOUT_PIN, INPUT);
  pinMode(SCK_PIN, OUTPUT);

  // pinMode(calib, INPUT_PULLUP);  // Set the reset pin as input with internal pull-up resistor(default state is high)
  // digitalWrite(calib, HIGH);
}

void loop() 
{  
  // put your main code here, to run repeatedly:

    if(digitalRead(DOUT_PIN) == 0)   // if dout low
	  {
		    int offset = read_average(10);
        Serial.println("offset");
        Serial.println(offset);
    }       
}






