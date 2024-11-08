

#include<HardwareSerial.h>

#define RXD2 16  //esp32 Rx
#define TXD2 17  //esp32 Tx

const int DOUT_PIN = 12;
const int SCK_PIN = 14;

int gain = 128, GAIN = 1;
int CMode = 0;

const int LVDT_pin = 25;  // Analog pin connected to LVDT output
float LVDT_voltage;       // Variable to store the voltage reading
float LVDT_position;      // Variable to store the calculated position
const int calib = 32;  // Set your reset button pin
int programState = 0,buttonState, c = 0;
long buttonMillis = 0;

// Define structures for reading data into.
signed long num = 0;

uint8_t data[3] = { 0 };
uint8_t filler = 0x00;
uint8_t tx_data[9];
float in_kg;

union FloatToBytes {
  float value;
  uint8_t bytes[4];
};
FloatToBytes converter;

int offset = -56693 , kg_weight = 39276, calib_weight = 1000;
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
         num = ((signed long)filler << 24) | ((signed long)data[2] << 16) | ((signed long)data[1] << 8) | (signed long)data[0];
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
///////////////  weight function
float weight_fun(int update, int off, int kg_wei)
{
  
  float res = (-off + update)*calib_weight/kg_wei;
  return res;
}

///////////////    received rxdata function
void rec_fun(uint8_t* data_rx, uint8_t count_rx)
{
  if(data_rx[0] == 1)
  {
    if(data_rx[1] == 0x03)
    {
      
      tx_data[0] = data_rx[0];  // slave id
      tx_data[1] = data_rx[1];  //fun code
      tx_data[2] = 4;        // no. of bytes to send
      
      for (int i = 0; i < 4; i++)
      {
        //Serial.print(converter.bytes[i], HEX);
        tx_data[3] = converter.bytes[3];      //data
        tx_data[4] = converter.bytes[2];      //data         
        tx_data[5] = converter.bytes[1];      //data     
        tx_data[6] = converter.bytes[0];      //data     
      }

      uint16_t crc = crc16(tx_data, 7);  // Compute CRC for tx_data[0] to tx_data[6]

      tx_data[7] = crc & 0xFF;       // Low byte of CRC
      tx_data[8] = (crc >> 8) & 0xFF;  // High byte of CRC


         // Print the data to Serial for debugging
            Serial.print("Sending: ");
            for (int i = 0; i < 9; i++) 
            {
                Serial.print(tx_data[i], HEX);
                Serial.print(" ");
            }
    }
  }
}





/* Table of CRC values for high-order byte */
static const uint8_t table_crc_hi[] = {
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const uint8_t table_crc_lo[] = {
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
  0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
  0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
  0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
  0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
  0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
  0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
  0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
  0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
  0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
  0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
  0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
  0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
  0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
  0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
  0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
  0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
  0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};






void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LVDT_pin, INPUT);
  pinMode(35, OUTPUT);  // RE & DE
  pinMode(34, OUTPUT);  // RE & DE

  Serial2.begin(19200, SERIAL_8E1, RXD2, TXD2);  // uart_begin
  //CMode = 1;

  pinMode(DOUT_PIN, INPUT);
  pinMode(SCK_PIN, OUTPUT);

  pinMode(calib, INPUT_PULLUP);  // Set the reset pin as input with internal pull-up resistor(default state is high)
  digitalWrite(calib, HIGH);
}

void loop() 
{  
  // put your main code here, to run repeatedly:
  //lvdt readings
  int sensorValue = analogRead(LVDT_pin); // Read the analog value from the LVDT
  // Serial.println("sensorValue");
  // Serial.println(sensorValue);

   LVDT_voltage = (sensorValue / 1023.0) * 3.2;  // Convert to voltage

  // // Apply a calibration formula for position (example, adjust based on your LVDT specs)
  // LVDT_position = (LVDT_voltage - 2.5) * 10; // Example: assuming 2.5V is center position

  Serial.print("Voltage: ");
  Serial.print(LVDT_voltage);
  delay(500);
  // Serial.print(" V, Position: ");
  // Serial.print(LVDT_position);
  // Serial.println(" mm");  // Adjust units based on your LVDT specs
  /////////////  calibration function
  while(CMode)
  {

    Serial.println("Entering calibration mode, remove known weight");
    delay(500);
    //should wait on while
    if(digitalRead(DOUT_PIN) == 0)   // if dout low
	  {
		    offset = read_average(10);
        Serial.println("offset");
        Serial.println(offset);
    }   
    Serial. println("put known weight and enter value");
    while(Serial.available() < 1) 
    {}
    if(Serial.available())
    {
      calib_weight = Serial.parseInt();
        Serial.println("calib weight");
        Serial.println(calib_weight);
      if(digitalRead(DOUT_PIN) == 0)   // if dout low
	    {
		    kg_weight = read_average(10)-offset;
        Serial.println("weight");
        Serial.println(kg_weight);
        CMode = 0;
      }   
    }
    
  }

  //////////////   receiving frame 
if (Serial2.available() > 0) 
  {
    uint8_t rx_count = 8;   // receiving frame {id, funcode, addr, no bytes, CRC} = 8bytes
    uint8_t RxData[8];
    Serial2.readBytes(RxData, rx_count);    
    // for(int x=0; x<rx_count; x++)
    // {
    //   Serial.println("hi");
    //   Serial.println(RxData[x]);
    // }
    rec_fun(RxData, rx_count);
    Serial2.write(tx_data, 9);
  }  

  /////////////////     weight knowing 
  if(digitalRead(DOUT_PIN) == 0)   // if dout low
  {
     long result = read_average(10);
      in_kg = weight_fun(result, offset, kg_weight); 
      converter.value = in_kg;
      Serial.print("weight: ");
      Serial.println((String)in_kg + " grams");
      //delay(4000);
  }

  /////////////////calib button 
  unsigned long currentMillis = millis();
  buttonState = digitalRead(calib);
  if (buttonState == LOW && programState == 0) 
  {
    buttonMillis = currentMillis;
    programState = 1;
  } 
  else if (programState == 1 && buttonState == HIGH) 
  {
    programState = 0;  //reset
  }
  if (((currentMillis - buttonMillis) > 3000) && programState == 1) 
  {
    programState = 0;
    //ledMillis = currentMillis;
    Serial.println(" entering into calibration mode");
    CMode = 1;
  }
}
//////////////    CRC Fun 
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length) 
{
  uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
  uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
  unsigned int i;        /* will index into CRC lookup */

  /* pass through message buffer */
  while (buffer_length--) {
    i = crc_lo ^ *buffer++; /* calculate the CRC  */
    crc_lo = crc_hi ^ table_crc_hi[i];
    crc_hi = table_crc_lo[i];
  }

  return (crc_hi << 8 | crc_lo);
}




