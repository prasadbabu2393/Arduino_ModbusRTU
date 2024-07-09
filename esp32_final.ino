
#include <HardwareSerial.h>
#include "stdint.h"
#include <WiFi.h>

#define RXD2 16       //esp32 Rx
#define TXD2 17        //esp32 Tx

//Address order: vol, curr, pf, kwh, kva 
//kwh is 64 bit 
uint32_t MHeaders[] = {502, 506, 508, 510, 506};
uint8_t Transmitidx = 0;
uint16_t Maddr=MHeaders[Transmitidx];
int ind = 0;
int retry_count = 0;                                     //reading holding 32 bit registers
byte TxData[] = {1, 3, 0x01, 0xF4, 0x00, 0x02};     //slave_id, fun code, start address, no. of bytes
uint8_t numRegs =TxData[5];                         // 1byte,     1 byte,     2 bytes ,     2 bytes
byte RxData[15];


typedef struct arr_strc                 //creating Data stucture
{
  uint8_t id;
  //vol, curr, pf, kwh, kva
  int64_t kwh;
  int64_t kva;
  float data[6];
}strc_arr;

volatile strc_arr dataStruct[1];

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

const char* network ="TP-Link_F237";
const char* password = "42191544";
//const char* network ="Mumma";
//const char* password = "8185861199";

WiFiServer server(80);
String header;
// Set IP address, Gateway IP address
// IPAddress local_IP(192, 168, 0, 184);
// IPAddress gateway(192, 168, 0, 1);
// IPAddress subnet(255, 255, 255, 0);

unsigned long currentTime = millis();    // Previous time
unsigned long previousTime = 0;   // Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

uint16_t crc16(uint8_t *buffer, uint16_t buffer_length);
void Transmit();
//int Display_1(uint8_t *Data_rx, uint8_t i, uint8_t Regs);

void Transmit(uint16_t addr)  ///byte TxData
{
  TxData[3] = byte(addr & 0xFF);
  TxData[2] = byte((addr >> 8)& 0xFF);
  if(Transmitidx==3)
  {
    TxData[5] = 0x04;
    numRegs = 0x04;
  }
  else
  {
    TxData[5] = 0x02;
    numRegs = 0x02;
  }
  // Serial.println("TxD ");
  // Serial.println(TxData[2]);
  // Serial.println(TxData[3]);
  uint16_t  abc = crc16(TxData, 6);
  TxData[6] = byte(abc & 0xFF); //lsb
  TxData[7] = byte((abc >> 8)& 0xFF); //msb
  

  digitalWrite(22, HIGH);  //transmitting
  digitalWrite(23, HIGH);

  Serial2.write(TxData, 8);
  Serial2.flush();

  digitalWrite(22, LOW);    //receiving
  digitalWrite(23, LOW);
  
}

void readRegs(uint8_t *Data_rx, uint8_t j, uint8_t Regs)      //serial printing of receiving data from available
{
  //Maddr = MHeaders[Transmitidx];
  if(Data_rx[0] == TxData[0])
  {
    if(Data_rx[1] == TxData[1])
    {
      if(Data_rx[2] == 0x08)
      {
        // if(Transmitidx == 3)
        // {
        dataStruct[0].kwh = ConvertINT64(Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6], Data_rx[7], Data_rx[8], Data_rx[9], Data_rx[10]);
        Serial.println(dataStruct[0].kwh);
       // }
      }
      else
      {
        dataStruct[0].data[Transmitidx] = convertToFloat(Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6]);
        Serial.println( dataStruct[0].data[Transmitidx]);
      }
    }
  }
  //(Transmitidx<=4) ? Transmitidx++ : 0;
  Transmitidx++;

  if(Transmitidx <= 4)
  {
    Maddr = MHeaders[Transmitidx];
    Transmit(Maddr);
  }
  else
  {
    ind = 0;
    Transmitidx = 0;
    Maddr = MHeaders[Transmitidx];
    Serial.println("data");
    for(int k=0; k<5; k++)
    {
      if(k == 3)
      {
        Serial.println(dataStruct[0].kwh);
      }
      else
      {
      Serial.println(dataStruct[0].data[k]);
      }
    }
  }
}

int64_t ConvertINT64(uint8_t Data3, uint8_t Data4, uint8_t  Data5, uint8_t Data6, uint8_t Data7, uint8_t Data8, uint8_t  Data9, uint8_t Data10)
{
  
    int64_t val_64 =  (int64_t(Data3<<56)) |((int64_t)Data4<<48) | ((int64_t)Data5<<40) | ((int64_t)Data6<<32) | ((int64_t)Data7<<24) | ((int64_t)Data8<<16) | ((int64_t)Data9<<8) | (int64_t)Data10;
    return val_64;
}

float convertToPF(uint8_t Data3, uint8_t Data4, uint8_t  Data5, uint8_t Data6 )
{
  float pf;
  if(Data3){pf = Data3;}
  if(abs(Data4)){pf = Data4;}
  if(abs(Data5)){pf = -2-Data5;}
  if(Data6){pf = 2-Data6;}
  
  return pf;
}


float convertToFloat(uint8_t Data3, uint8_t Data4, uint8_t  Data5, uint8_t Data6 )
{
    uint8_t sign = Data3 & 0x80;
    uint8_t expo = (Data3 << 1) | (Data4 >> 7);
    uint32_t  m = ((Data4 & 0x7F)<<16) | (Data5 <<8) | Data6 ;
    float val = 0;

    for (int p = 22; p >= 0; --p) 
    {
        if (m & (1 << p)) 
        {
            val += pow(2, p - 23); // Shift by 23 because mantissa is 23 bits
        }
    }
    val += 1;
    val = val * (pow(2, expo-127));  // * (pow(-1,sign));
    if(sign)
    {
      val = -val;
    }
  
    return val;
}

uint16_t crc16(uint8_t *buffer, uint16_t buffer_length)
{
    uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_lo ^ *buffer++; /* calculate the CRC  */
        crc_lo = crc_hi ^ table_crc_hi[i];
        crc_hi = table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}

void setup() 
{
  // setup code parameters to run once:
  pinMode(22, OUTPUT); // RE & DE
  pinMode(23, OUTPUT); // RE & DE

  Serial2.begin(19200, SERIAL_8E1, RXD2, TXD2);
  Serial.begin(9600);
  Transmit(Maddr);
  WiFi.begin(network, password);

  // if (!WiFi.config(local_IP, gateway, subnet)) 
  // {
  //   Serial.println("STA Failed to configure");
  // }

  while(WiFi.status() != WL_CONNECTED)
  {
  Serial.println("connecting...");
  delay(1000);
  retry_count++;
  if(retry_count>10){
    ESP.restart();
  }
  }
  Serial.println("connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
   
  server.begin();
   
}

void loop() 
{
  // put your main code here, to run repeatedly:
 if(Serial2.available() > 0)
 {
  int rx_count = (numRegs*2) + 5;       //total no. of bytes to be read 
  Serial2.readBytes(RxData, rx_count);
  // for(int x=0; x<rx_count; x++)
  // {
  // Serial.println(RxData[x]);
  // }
  
  readRegs(RxData, ind, numRegs);
  ind++;
  //(ind<=4) ? ind++ : ind=0;
  Serial.println(ind);
  
 }

 WiFiClient client = server.available(); 
 if(client)
  {
    currentTime = millis();
    previousTime = currentTime;
    //Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = ""; 
    while(client.connected() && currentTime - previousTime <= timeoutTime)
    {
      currentTime = millis();
      if(client.available())
      {
        char c= client.read();
        //Serial.write(c);
        header +=c;
        if(c == '\n')
        {
          if(currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();         
            //client.println("Hello World");
            if (header.indexOf("GET /EMDaa") >= 0) 
            {
              Serial.println("GPIO 26 on");
              //Transmit(Maddr);
              //i = 0;
            }
            else if(header.indexOf("GET /readdata") >= 0)
            {
              //Maddr = 500             
              Transmit(Maddr);
            }
            // else if(header.indexOf("GET /var") >= 0){
            //   Transmit(502);
            // }
            // else if(header.indexOf("GET /curr") >= 0){
            //   Transmit(504);
            // }
            else 
            {                           
                Serial.println("client");   
                client.print(" {id: ");
                client.print(dataStruct[0].id);
                client.print(",");

                for(int  j=0; j<5; j++)
                {
                  if(j==3)
                  {
                    client.print(dataStruct[0].kwh); 
                    client.print(", "); 
                  }
                  else
                  {
                  client.print(dataStruct[0].data[j]);
                  client.print(", ");
                  }
                }
                client.println("}");              
            }
            break;
          }
          else
          {
            currentLine ="";
          }
        }
        else if (c != '\r')
        {
          currentLine +=c;
        }
      }
    }
    header = "";
    // Close the connection
    client.stop();
    //Serial.println("Client disconnected.");
    //Serial.println("");
  }
}
