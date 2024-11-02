
#include<HardwareSerial.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Preferences preferences;

#define I2C_SDA 33  // i2c for 1st sensor
#define I2C_SCL 32  
#define SDA_1 27 // i2c for 2nd sensor
#define SCL_1 26
#define RXD2 16  //esp32 Rx for stm32
#define TXD2 17  //esp32 Tx for stm32
#define UART_TX_PIN 4  // Custom TX pin   // Define custom UART pins
#define UART_RX_PIN 5  // Custom RX pin
#define SEALEVELPRESSURE_HPA (1013.25)

HardwareSerial mySerial(1);  // Use UART1 (you can use UART2 if you want another hardware UART)
TwoWire I2CBME = TwoWire(0); //custom pin for i2c
TwoWire I2Ctwo = TwoWire(1); // //custom pin for i2c
Adafruit_BME280 bme;  //object for bme sensor header file

int cof_count =0 ;
float cof_arr[1000];
bool SAMPLE =0;
String header ="";
WiFiServer server(80);
int cyclescount = 0;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

const char* ssid_h = "ESP32_Load";  // hotpot name and password  ACT102646054057-5g   16761789
const char* password_h = "12345";     // 192.168.0.111  -5g   // 192.168.0.114

const int DOUT_PIN = 12; // dout pin for hx711
const int SCK_PIN = 14;  // sck pin for hx711
const int calib = 34;  // Set your calibration button pin

int32_t number ;
uint8_t RxData[100];
int a1 =0;
float cof=0;
int brake_count =0;
int present_state, prev_state;

unsigned long delayTime;
int offset = -56693 , kg_weight = 39270, calib_weight = 1000; //calculation for weight in kg's
int gain = 128, GAIN = 1; //header file values taken from hx711 header file
int CMode = 0;
int programState = 0,buttonState, c = 0;
long buttonMillis = 0;
float grams_fun;

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

// shifting function fortaking 24 bits 0 or 1 by dout pin or pulses
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

// read function for taking 24 bits and converting into 32 bits 
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

///this function for calculating average of 10 readings of sensor
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
///this function for calculating weight in grams 
float weight_fun(int update, int off, int kg_wei)
{
  
  float res = (-off + update)*calib_weight/kg_wei; 
  return res;
}

// this function for taking count of A,B from sm32 and converting received bytes into integer(+/-)
int32_t fromBytesToSignedInt(uint8_t* byteArray) {
  return (int32_t)(byteArray[3] | 
                   (byteArray[2] << 8) | 
                   (byteArray[1] << 16) | 
                   (byteArray[0] << 24));
}

// this function for printing bme280 sensor readings
void printValues() 
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());// taking temperature value from bme sensor
  Serial.println(" *C");
  
  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/
  
  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F); // taking pressure value from bme sensor
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));// taking altitude value from bme sensor
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());// taking humidity value from bme sensor
  Serial.println(" %");

  Serial.println();
}

//this function for printing IR temperature from hw-691 sensor
void mlx_data()
{
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC());  //ambient temperature in celsius
  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");//object temperature in celsius
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempF()); //ambient temperature in fahernheit
  Serial.print("*F\tObject = "); Serial.print(mlx.readObjectTempF()); Serial.println("*F");//object temperature in fahernheit
  Serial.println();

}

////this functio when calibration button pressed
  void calib_button()
  {
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

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(DOUT_PIN, INPUT); //taking dout pin as input 
  pinMode(SCK_PIN, OUTPUT); //taking sck pin as output 

  pinMode(calib, INPUT_PULLUP);  // Set the reset pin as input with internal pull-up resistor(default state is high)
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);  // uart_begin
  mySerial.begin(19200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);  // UART on custom pin

  preferences.begin("memory", false);

  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  I2Ctwo.begin(SDA_1, SCL_1, 100000);
  Serial.println("Adafruit MLX90614 test");   
   mlx.begin(0x5A, &I2Ctwo); 

  Serial.println(F("BME280 test"));
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);

  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid_h, password_h);
  server.begin();

  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76, &I2CBME);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
   // while (1);
  }

  Serial.println("-- Default Test --");
  //delayTime = 1000;

  prev_state = digitalRead(34);
}

void loop() 
{  
  // put your main code here, to run repeatedly:


  //  if (mySerial.available() >0) //change according to number bytes receiving
  //   {
  //     Serial2.readBytes(); 
  //   }
//taking A,B count bytes from stm32 
  if (Serial2.available() >= 4) //change this 4 according to number bytes receiving
    {
       Serial.println("--"); 
       a1 = Serial2.available(); 
     //Serial.println(a1);
       Serial2.readBytes(RxData, a1); // reading bytes
       number = fromBytesToSignedInt(RxData);
        Serial.println("count");
        Serial.println(number);
      //  for(int x=0; x<a1; x++)     
      //  {
      //    Serial.println("byte" + (String)x);
      //    Serial.println(RxData[x]);
      //  } 

  }  
//  /////////////  calibration function
//   while(CMode)
//   {
//     Serial.println("Entering calibration mode, remove known weight");
//     delay(500);
//     //should wait on while
//     if(digitalRead(DOUT_PIN) == 0)   // if dout low
// 	  {
// 		    offset = read_average(10);// taking average without weight
//         Serial.println("offset");
//         Serial.println(offset);
//     }   
//     Serial. println("put known weight and enter value");
//     while(Serial.available() < 1) // user should enter known weight(1kg) value in serial mpnitor
//     {}
//     if(Serial.available())
//     {
//       calib_weight = Serial.parseInt();

//         Serial.println("calib weight");
//         Serial.println(calib_weight);
//       if(digitalRead(DOUT_PIN) == 0)   // if dout low
// 	    {
// 		    kg_weight = read_average(10)-offset;
//         Serial.println("weight");
//         Serial.println(kg_weight);
//         CMode = 0;
//       }   
//     }
//   }

  ///printing weight  
  int present_state = digitalRead(34);
   
  if(present_state == 0 && prev_state == 1)
  {
   SAMPLE =1;
   brake_count++; //incrementing brake count how times switch pressed
   //Serial.println("sample");
  //  Serial.println("brake_count");
  //  Serial.println(brake_count);
   preferences.putString("brake_count", (String)brake_count); //storing the brake count into flash
   if(digitalRead(DOUT_PIN) == 0)   // if dout low
   {
     long result = read_average(10);
      in_kg = weight_fun(result, offset, kg_weight); 
      converter.value = in_kg;
      Serial.print("weight: ");
      Serial.println((String)in_kg + " grams");
      cof = in_kg/5000; //5000 because weight given 
      Serial.println("cof:");
      Serial.println(cof);
   }
  }
  else if(present_state == 1 && prev_state == 0) //when switch state changes from 0 to 1
  {
    preferences.putInt("cof_count", cof_count); //storing al cof count values into flash memory
    for(int a=0; a<cof_count; a++)
    {
      preferences.putString(("cof" + (String)a).c_str(), (String)cof_arr[a]); //storing al cof values into flash memory
    }
    // Serial.println("cof_count");
    // Serial.println(cof_count);
    SAMPLE = 0; //making zero for next time 
    cof_count =0; //making zero for next time 
  }
   //calib_button();
   server_code();
   prev_state = present_state; //storing the switch state in another variable

   // when switch is in pressed state only calculating cof values and storing into array
   if(SAMPLE == 1)
   {
     if(digitalRead(DOUT_PIN) == 0)   // if dout low
     {
      long result = read_average(10);
      in_kg = weight_fun(result, offset, kg_weight); 
      converter.value = in_kg;
      Serial.print("weight: ");
      Serial.println((String)in_kg + " grams");
       cof = in_kg/5000;
      // preferences.putString(("cof" + (String)brake_count).c_str(), (String)cof);
      Serial.println("cof:");
      Serial.println(cof);
      cof_arr[cof_count] = cof; //storing into a arrey for cof values
      cof_count++; //taking count of cof values when switch is in pressed state only
     }
   }
}

 ///////////  wifi server code
void server_code()
{
  WiFiClient client = server.available(); 
  if(client)
  {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = ""; 
    while(client.connected() && currentTime - previousTime <= timeoutTime)
    {
      currentTime = millis();
      if(client.available())
      {
        char c = client.read();
        header += c;
        if(c == '\n')
        {
          if(currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();         
            if (header.indexOf("GET /data") >= 0) 
            {
              client.print("{");
              client.print(cof); //printing cof value calculated by weight
              client.print(",");
              client.print(number); //printing the count of AB pulses received from stm32
              client.print(",");
              client.print(digitalRead(34)); //printing of status of switch
              client.print(",");
              client.print(brake_count); //printing how times switch pressed
              client.print(",");
              client.print(mlx.readObjectTempC()); //object temperature in celsius
              client.print(",");
              client.print(bme.readTemperature());// taking temperature value from bme sensor
              client.print(",");
              client.print(bme.readPressure() / 100.0F); // taking pressure value from bme sensor
              client.print(",");
              client.print(bme.readAltitude(SEALEVELPRESSURE_HPA));// taking altitude value from bme sensor
              client.print(",");
              client.print(bme.readHumidity());// taking humidity value from bme sensor
              client.print("}");
              break;
            }
            else if(header.indexOf("GET /reset") >= 0)
            {
              uint8_t a1 =0;
              Serial2.write(a1); // sending 0 to stm32 to make A,B count =0 
              brake_count =0 ;
              number =0 ;
              cof = 0;
              preferences.clear();
              break;
            }
            else if(header.indexOf("GET /restart") >= 0)
            {
              uint8_t b1 =1;
              Serial2.write(b1);// sending 1 to stm32 for reset 
              ESP.restart(); //esp32 will restart
              break;
            }
            else if(header.indexOf("GET /cof") >= 0)
            {
              //preferences.
              int j = preferences.getInt("cof_count", 0); //reading the number of count of cofs stored in flash memory 
              // Serial.println("j");
              // Serial.println(j);
              String a[j]; //creating string array
              for(int x=0; x<j; x++)
              {
                a[x] = preferences.getString(("cof" + (String)(x)).c_str(), ""); //reading the cofs from flash memory
                client.print(a[x]); // printing cofs in client
               // Serial.println(a[x]);
                client.print(",");
              }
              preferences.clear(); //ater printing clearing all flash memory
              break;
            }
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
  }
}


