 
#include <HardwareSerial.h>
#include "stdint.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>

WiFiServer server(80);
String header;
Preferences preferences;
Preferences preferences_1;
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// MQTT Broker
const char *mqtt_broker = "192.168.0.193";
const char *topic = "ethernet";
String mqtt_client_id = "ethernet";
String publish_data = "";
const int mqtt_port = 1883;

#define RXD2 16  //esp32 Rx
#define TXD2 17  //esp32 Tx


bool MODE, PUBLISH;
String ssid, password, ip;
String network_1, password_1, client_ip;
String voltage, current, pf, kva, kwh;
String Add_pre[5], Add_flash[5];

const char *ssid_h = "ESP32";
const char *password_h = "42191544";
//const char* network ="Touchstone";
//const char* password = "secure@123";

int DE_RE_pin = 19;   // DE and RE pin
int ledpin = 5;       // led pin
const int resetPin = 21;  // Set your reset button pin
int programState = 0, buttonState;
long buttonMillis = 0 ;
int retry_count = 0;
int count =0;
//long start_time =0;
long prev_time =0;
unsigned long currentTime = millis();  // Previous time
unsigned long previousTime = 0;        // Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//Address order: vol, curr, pf, kwh, kva
//kwh is 64 bit
uint32_t MHeaders[5];   // = { 502, 506, 508, 510, 506 };
//  uint32_t MHeaders[] = { 502, 506, 508, 510, 506 };
//uint32_t MHeaders[] = {3025, 3009, 3083, 3203, 3235};
uint8_t Transmitidx = 0;
uint16_t Maddr;
uint8_t mqtt_retry_count = 0;


////////////     static IP
int a = 192, b = 168, c = 0, d = 118;
// Set your Static IP address
 // 192.168.0.114
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);  ///1 -- 3 REPLACED
IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional
IPAddress flash_ip;

///////////////   function declarations ///////////
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length);
void Transmit();
//int Display_1(uint8_t *Data_rx, uint8_t i, uint8_t Regs);

///////////////////////////////    reading holding 32 bit registers     ///////////////////////////////////
byte TxData[] = { 1, 3, 0x01, 0xF4, 0x00, 0x02 };  //slave_id, fun code, start address, no. of bytes
uint8_t numRegs = TxData[5];                       // 1byte,     1 byte,     2 bytes ,     2 bytes
byte RxData[15];

//////////////////    creating Data stucture
typedef struct arr_strc  
{
  uint8_t id;
  //vol, curr, pf, kwh, kva
  int64_t kwh;
  int64_t kva;
  float data[6];
} strc_arr;

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
void periodic_send_5sec(long );
void periodic_send_5sec(long start_time)
{
   if(start_time - prev_time > 5000) //sending tx frames to slave for every 5 seconds
  {
    Serial.println("transmitting frames for 5sec");
    Transmit(Maddr);
    PUBLISH = 1; 
    prev_time  = start_time;
  }
}

//this fun is for send the switch states and counts to mqtt broker
void publish()
{
   if (mqtt_client.connected()) // connecting to mqtt broker
   {
      Serial.println("In publish:");
      publish_data = "";
      Serial.println("publish data");
      for (int k = 0; k < 5; k++) {
       if (k == 3) {
        publish_data += (String)(dataStruct[0].kwh) + ",";
       } else if (k == 4) {
        publish_data += (String)(dataStruct[0].kva) + ",";
       } else {
        publish_data += (String)(dataStruct[0].data[k]) + ",";
       }
      }
       Serial.println(publish_data);
      mqtt_client.publish(topic, publish_data.c_str());  //sending switchs data to mqtt  broker
      PUBLISH = 0;
    }
}

//this fun is for mqtt server retry connection to broker when connection lost after connected to broker
void reconnect()
{
       while (!mqtt_client.connected()) 
       {
          mqtt_retry_count++;
         if(mqtt_retry_count > 10)
         {
           ESP.restart();
          }
          //String mqtt_client_id = "esp32";
          if (mqtt_client.connect(mqtt_client_id.c_str())) 
          {
            Serial.println("MQTT broker connected");
            break;
             // Once connected, subscribe and publish to the topic
            //mqtt_client.publish(topic, "Hi, I'm ESP32 ^^");
          } 
          else 
          {
            Serial.print("failed with state ");
            Serial.print(mqtt_client.state());
          }
          hard_reset_button();
          wifi_server_code();
          periodic_send_5sec(millis()); // transmitting 5 tx frames for every 5 seconds
          save_to_flash(); //saving received data into falsh memory
        }
}

//callback function for mqtt server 
void callback(char *topic, byte *payload, unsigned int length) //topic[intialized], payload = string from broker and its length
{
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) 
    {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void save_to_flash() //function for saving data when no wifi and mqtt
{
  publish_data = "";
      Serial.println("publish data");
      for (int k = 0; k < 5; k++) {
       if (k == 3) {
        publish_data += (String)(dataStruct[0].kwh) + ",";
       } else if (k == 4) {
        publish_data += (String)(dataStruct[0].kva) + ",";
       } else {
        publish_data += (String)(dataStruct[0].data[k]) + ",";
       }
      }
   count++;
   preferences_1.putString(("count" + (String)count).c_str(), publish_data);  // data stored into flash when no wifi or mqtt
   preferences_1.putString("count", (String)count);  //count for number of times 5 data values stored into flash memory
   preferences_1.end();
}

void setap()
{
    /////////////////////     set ap
        MODE = 0;
       Serial.print("Setting AP (Access Point)…");
        WiFi.softAP(ssid_h, password_h);
       IPAddress IP = WiFi.softAPIP();
       Serial.print("AP IP address: ");
       Serial.println(IP); 
       server.begin();
       digitalWrite(ledpin,!digitalRead(ledpin));
}

void hard_reset_button() // function for hard reset of esp32
{
///////////  reset button function
  unsigned long currentMillis = millis();
  buttonState = digitalRead(resetPin);
  if (buttonState == LOW && programState == 0) {
    buttonMillis = currentMillis;
    programState = 1;
  } else if (programState == 1 && buttonState == HIGH) {
    programState = 0;  //reset
  }
  if (((currentMillis - buttonMillis) > 3000) && programState == 1) {
    programState = 0;
    //ledMillis = currentMillis;
    Serial.println(" reseting and clearing credencials");
    preferences.clear();
    preferences.end();
    esp_restart();
  }
}

// function for replacing string with special chars              
String urlDecode(String input) // URL decode function
{
              input.replace("%20", " ");
              input.replace("%21", "!");
              input.replace("%22", "\"");
              input.replace("%23", "#");
              input.replace("%24", "$");
              input.replace("%25", "%");
              input.replace("%26", "&");
              input.replace("%27", "'");
              input.replace("%28", "(");
              input.replace("%29", ")");
              input.replace("%2A", "*");
              input.replace("%2B", "+");
              input.replace("%2C", ",");
              input.replace("%2D", "-");
              input.replace("%2E", ".");
              input.replace("%2F", "/");
              input.replace("%40", "@"); 
              // Add more replacements as needed
              return input;
}   

//  Transmit function for transmitting modbus tx frames with 5 different addresses simultaniously
void Transmit(uint16_t addr)  ///byte TxData
{
  TxData[2] = byte((addr >> 8) & 0xFF);
  TxData[3] = byte(addr & 0xFF);
 
  if (Transmitidx == 3 | Transmitidx == 4) 
  {
    TxData[5] = 0x04;
    numRegs = 0x04;
  }
  else 
  {
    TxData[5] = 0x02;
    numRegs = 0x02;
  }
  uint16_t abc = crc16(TxData, 6);
  TxData[6] = byte(abc & 0xFF);         //lsb
  TxData[7] = byte((abc >> 8) & 0xFF);  //msb


  digitalWrite(DE_RE_pin, HIGH);  //transmitting
  //digitalWrite(23, HIGH);

  Serial2.write(TxData, 8);
  Serial2.flush();

  digitalWrite(DE_RE_pin, LOW);  //receiving
  //digitalWrite(23, LOW);
}

//ReadRegs function for converting received bytes from slave into float/int32/64 according to address
void readRegs(uint8_t *Data_rx, uint8_t j, uint8_t Regs)  //serial printing of receiving data from available
{
  //Maddr = MHeaders[Transmitidx];
  if (Data_rx[0] == TxData[0]) {
    if (Data_rx[1] == TxData[1]) {
      if (Data_rx[2] == 0x08) {
        if (Transmitidx == 3) {
          dataStruct[0].kwh = ConvertINT64(Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6], Data_rx[7], Data_rx[8], Data_rx[9], Data_rx[10]);
          //Serial.println(dataStruct[0].kwh);
        } else if (Transmitidx == 4) {
          dataStruct[0].kva = ConvertINT64(Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6], Data_rx[7], Data_rx[8], Data_rx[9], Data_rx[10]);
        }
      } else {
        if (Transmitidx == 2) {
          dataStruct[0].data[Transmitidx] = convertToPF(Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6]);
          Serial.println(dataStruct[0].data[Transmitidx]);
        } else {
          dataStruct[0].data[Transmitidx] = convertToFloat(Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6]);
          Serial.println(dataStruct[0].data[Transmitidx]);
        }
      }
    }
  }
  //(Transmitidx<=4) ? Transmitidx++ : 0;
  Transmitidx++;
  if (Transmitidx <= 4) {
    Maddr = MHeaders[Transmitidx];
    Transmit(Maddr);
  } else {
    //ind = 0;
    Transmitidx = 0;
    Maddr = MHeaders[Transmitidx];
    Serial.println("data");
    for (int k = 0; k < 5; k++) {
      if (k == 3) {
        Serial.println(dataStruct[0].kwh);
      } else if (k == 4) {
        Serial.println(dataStruct[0].kva);
      } else {
        Serial.println(dataStruct[0].data[k]);
      }
    }
  }
}

//ConvertINT64 functon for receiving 8 bytes from slave converting into int64 value
int64_t ConvertINT64(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6, uint8_t Data7, uint8_t Data8, uint8_t Data9, uint8_t Data10) {

  int64_t val_64 = ((int64_t)Data3 << 56) | ((int64_t)Data4 << 48) | ((int64_t)Data5 << 40) | ((int64_t)Data6 << 32) | ((int64_t)Data7 << 24) | ((int64_t)Data8 << 16) | ((int64_t)Data9 << 8) | (int64_t)Data10;
  return val_64;
}

//convertToPF functon for receiving 4 bytes from slave converting into pf value according to slave datasheet 
float convertToPF(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) {
  float pf = 0.0, regval = 0.0;
  regval = convertToFloat(Data3, Data4, Data5, Data6);

  if (regval > 1.0) {
    pf = 2 - regval;
  } else if (regval < -1.0) {
    pf = -2 - regval;
  } else if (abs(regval) == 1) {
    pf = regval;
  } else {
    pf = regval;
  }

  // if(Data3){pf = Data3;}
  // if(abs(Data4)){pf = Data4;}
  // if(abs(Data5)){pf = -2-Data5;}
  // if(Data6){pf = 2-Data6;}

  return pf;
}

//convertToFloat functon for receiving 4 bytes from slave converting into float value according to IEEE format
float convertToFloat(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) 
{
  uint8_t sign = Data3 & 0x80;
  uint8_t expo = (Data3 << 1) | (Data4 >> 7);
  uint32_t m = ((uint32_t)(Data4 & 0x7F) << 16) | ((uint32_t)Data5 << 8) | Data6;
  float val = 0;

  for (int p = 22; p >= 0; --p) {
    if (m & (1 << p)) {
      val += pow(2, p - 23);  // Shift by 23 because mantissa is 23 bits
    }
  }
  val += 1;
  val = val * (pow(2, expo - 127));  // * (pow(-1,sign));
  if (sign) {
    val = -val;
  }

  return val;
}

// crc16 functon for calculating crc for every transmision frames
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

// string IP to int a,a b, c, d variables function
void convert(String address)
{
  // Split the IP address string by dots
  int d1 = address.indexOf('.');
  int d2 = address.indexOf('.', d1 + 1);
  int d3 = address.indexOf('.', d2 + 1);

  // Convert each part into an integer
  a = address.substring(0, d1).toInt();
  b = address.substring(d1 + 1, d2).toInt();
  c = address.substring(d2 + 1, d3).toInt();
  d = address.substring(d3 + 1).toInt();

  // Initialize local_IP with the integers from flash_ip
  //local_IP(a, b, c, d);
}

void wifi_server_code()  // server code
{
 
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    //Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        header += c;
        if (c == '\n') 
        {
          if (currentLine.length() == 0) 
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            //client.println("Hello World");
            if (header.indexOf("GET /readdata") >= 0) 
            {
              client.print("readdata");
              //Maddr = 500
              Transmit(Maddr);
              Serial.println("readdata");
            }

            else if (header.indexOf("GET /getdata") >= 0) 
            {
              Serial.println("client");
              client.println("client");
              client.print(" {");
              //client.print(dataStruct[0].id);
              client.print(",");
              for (int j = 0; j < 5; j++) {
                if (j == 3) {
                  client.print(dataStruct[0].kwh);
                  client.print(", ");
                } else if (j == 4) {
                  client.print(dataStruct[0].kva);
                  client.print(", ");
                } else {
                  client.print(dataStruct[0].data[j]);
                  client.print(", ");
                }
              }
              client.println("}");
              break;
            }

            else if (header.indexOf("GET /pwd") >= 0) { // ip address with password eg: 192.168.0.181/pwd"password" [no need ""]
              int pwdStartIndex = header.indexOf("GET /pwd?") + 9;
              int pwdEndIndex = header.indexOf(' ', pwdStartIndex);
              if (pwdEndIndex == -1) {
                // Handle case where there's no space after ssid
                pwdEndIndex = header.length();
              }
              String pwdValue = header.substring(pwdStartIndex, pwdEndIndex);
              Serial.println("PWD: " + pwdValue);
              uint8_t value = pwdEndIndex - pwdStartIndex;
              preferences.putString("password", pwdValue);
              // Serial.println(value);
              //write to flash
              ESP.restart();
            }


            else if (header.indexOf("GET /ssid") >= 0) { // ip address with ssid eg: 192.168.0.181/ssid"username" [no need ""]
              int ssidStartIndex = header.indexOf("GET /ssid?") + 10;
              int ssidEndIndex = header.indexOf(' ', ssidStartIndex);
              if (ssidEndIndex == -1)  // Handle case where there's no space after ssid
              {
                ssidEndIndex = header.length();
              }
              String ssidValue = header.substring(ssidStartIndex, ssidEndIndex);
              Serial.println(ssidStartIndex);
              Serial.println(ssidEndIndex);
              uint8_t value_1 = (ssidEndIndex - ssidStartIndex);
              preferences.putString("ssid", ssidValue);
              ESP.restart();
              //Serial.println(value_1);
            }
            /////////////    MAc address
             else if (header.indexOf("GET /mac") >= 0) 
             {
                client.println(WiFi.macAddress());
             }
              /////////////   esp restart
             else if (header.indexOf("GET /restart") >= 0) 
             {
                ESP.restart();
             } 
              /////////////  mqtt topic
             else if (header.indexOf("GET /topic") >= 0) 
             {
                client.println(topic);
             } /////////////    wifi status
             else if (header.indexOf("GET /mqttstatus") >= 0) 
             {
                if (mqtt_client.connected()) 
                {
                  client.println("0");
                }
                else
                {
                  client.println("1");
                }
             }
             /////////////    Modbus address
             else if (header.indexOf("GET /modbusaddress") >= 0) 
             {
                Add_flash[0] = preferences.getString("Address_1", "");  //voltage address
                Add_flash[1] = preferences.getString("Address_2", "");  //current address
                Add_flash[2] = preferences.getString("Address_3", "");  //pf address
                Add_flash[3] = preferences.getString("Address_4", "");  //kva address
                Add_flash[4] = preferences.getString("Address_5", "");  //kwh address
                // printing addresses after reading from flash
                for (int d = 0; d < 5; d++) 
                {
                   client.println((String)Add_flash[d] + ","); 
                }
             }  
             //////  printing info, saved data when no wifi or mqtt
             else if (header.indexOf("GET /saveddata") >= 0) 
             {
                count =  preferences_1.getString("count", "").toInt(); //reading count, number of times data is saved from flash
                String s[count] ={""};
                for(int y =0; y<count; y++)
                {
                  preferences_1.getString(("count" +(String)y).c_str(), "");  //reading from flash
                }
             }
               //////  clearing the saved data when no wifi or mqtt
             else if (header.indexOf("GET /cleardata") >= 0) 
             {
                preferences_1.clear();
             }
            ////////////      login page
            else if (header.indexOf("GET /login") >= 0) {
              Serial.println("Logging in");
              // Display the HTML web page
              client.println("<!DOCTYPE html>");
              client.println("<html>");
              client.println("<body>");
              client.println("<form action=\"/action_page.php\">");

              /////////////  ssid and password 
              client.println("<label for=\"ssid\">ssid   : </label>");
              client.println(" <input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>");
              client.println("<label for=\"password\">Password   : </label>");
              client.println("<input type=\"text\" id=\"password\" name=\"password\"><br><br>");

              ////////////////////  static IP
              client.println("<p> enter IP address like eg: 192.168.0.1 </p>");
              client.println("\n");
              client.println("<label for=\"static_IP\">static IP : </label>");
              client.println("<input type=\"text\" id=\"static_IP\" name=\"static_IP\"><br><br>");

              ///////////////////    5 addresses 
              client.println("<label for=\"voltage\">voltage : </label>");
              client.println("<input type=\"text\" id=\"voltage\" name=\"voltage\"><br><br>");
              client.println("<label for=\"current\">current : </label>");
              client.println("<input type=\"text\" id=\"current\" name=\"current\"><br><br>");
              client.println("<label for=\"pf\">pf : </label>");
              client.println("<input type=\"text\" id=\"pf\" name=\"pf\"><br><br>");
              client.println("<label for=\"kva\">kva : </label>");
              client.println("<input type=\"text\" id=\"kva\" name=\"kva\"><br><br>");
              client.println("<label for=\"kwh\">kwh : </label>");
              client.println("<input type=\"text\" id=\"kwh\" name=\"kwh\"><br><br>");

              ////////////////   submit button
              client.println("<input type=\"submit\" value=\"Submit\">");
              client.println("</form>");

              //////////////   script function
              client.println("<script>");
              client.println("function encodeFormData() {");
              client.println("const form = document.forms[0];");
              client.println("form.ssid.value = encodeURIComponent(form.ssid.value);");
              client.println("form.password.value = encodeURIComponent(form.password.value);");
              client.println("form.static_IP.value = encodeURIComponent(form.static_IP.value);");
              client.println("form.voltage.value = encodeURIComponent(form.voltage.value);");
              client.println("form.current.value = encodeURIComponent(form.current.value);");
              client.println("form.pf.value = encodeURIComponent(form.pf.value);");
              client.println("form.kva.value = encodeURIComponent(form.kva.value);");
              client.println("form.kwh.value = encodeURIComponent(form.kwh.value);");
              client.println("}");
              client.println("</script>");
              //////////////

              client.println("<p>Click on the submit button to submit the form.</p>");
              client.println("</body></html>");
              client.println();
              // The HTTP response ends with another blank line
              break;
            }
            ///////////////   after submit seperating details
            else if (header.indexOf("GET /action_page.php") >= 0) 
            {
             
              int networkStart = header.indexOf("ssid=") + 5;
              int networkEnd = header.indexOf("&", networkStart);
              network_1 = header.substring(networkStart, networkEnd);
              network_1 = urlDecode(network_1);  // Decode SSID

               // Extract password
              int passwordStart = header.indexOf("password=") + 9;
              int passwordEnd = header.indexOf("&", passwordStart);
              password_1 = header.substring(passwordStart, passwordEnd);
              password_1 = urlDecode(password_1);  // Decode password

              // Extract static IP
             int ipStart = header.indexOf("static_IP=") + 10;
             int ipEnd = header.indexOf("&", ipStart);
             client_ip = header.substring(ipStart, ipEnd);
             client_ip = urlDecode(client_ip);  // Decode static IP

            // Extract voltage
            int AddressStart_1 = header.indexOf("voltage=") + 8;
            int AddressEnd_1 = header.indexOf("&", AddressStart_1);
            voltage = header.substring(AddressStart_1, AddressEnd_1);
            voltage = urlDecode(voltage);  // Decode voltage

            // Extract current
            int AddressStart_2 = header.indexOf("current=") + 8;
            int AddressEnd_2 = header.indexOf("&", AddressStart_2);
            current = header.substring(AddressStart_2, AddressEnd_2);
            current = urlDecode(current);  // Decode current

            // Extract pf
            int AddressStart_3 = header.indexOf("pf=") + 3;
            int AddressEnd_3 = header.indexOf("&", AddressStart_3);
            pf = header.substring(AddressStart_3, AddressEnd_3);
            pf = urlDecode(pf);  // Decode pf

             // Extract kva
            int AddressStart_4 = header.indexOf("kva=") + 4;
            int AddressEnd_4 = header.indexOf("&", AddressStart_4);
            kva = header.substring(AddressStart_4, AddressEnd_4);
            kva = urlDecode(kva);  // Decode kva

            // Extract kwh
            int AddressStart_5 = header.indexOf("kwh=") + 4;
            int AddressEnd_5 = header.indexOf(" ", AddressStart_5);
            kwh = header.substring(AddressStart_5, AddressEnd_5);
            kwh = urlDecode(kwh);  // Decode kwh

              // Break out of the while loop
              Serial.println("header");
              Serial.println(header);
              Serial.println("network : ");
              Serial.println(network_1);
              Serial.println("password : ");
              Serial.println(password_1);
              Serial.println("client_iP");
              Serial.println(client_ip);

              Add_pre[0] = voltage;
              Add_pre[1] = current;
              Add_pre[2] = pf;
              Add_pre[3] = kva;
              Add_pre[4] = kwh;
              Serial.println("client addresses");
              for (int o = 0; o < 5; o++) {
                Serial.println(Add_pre[o]);
              }
              //preferences.begin("credentials", false);
            
               if (network_1 != "")
                {
                  preferences.putString("ssid", network_1);  // ssid storing into flash
                }
                if (password_1 != "")
                {
                   preferences.putString("password", password_1);  // paasword storing into flash
                }
                if (client_ip != "")
                {
                   preferences.putString("staticIP", client_ip); // static_IP storing into flash
                }
              
                for (int k = 0; k < 5; k++) 
                {
                  if (Add_pre[k] != "") 
                  {
                    //Serial.println(Add_pre[k]);
                    switch (k) 
                    {
                        ///////////      storing into the flash           ///////////
                      case 0: preferences.putString("Address_1", voltage); break;  //Address 1
                      case 1: preferences.putString("Address_2", current); break;  //Address 2
                      case 2: preferences.putString("Address_3", pf); break;  //Address 3
                      case 3: preferences.putString("Address_4", kva); break;  //Address 4
                      case 4: preferences.putString("Address_5", kwh); break;   //Address 5
                    }            
                  }
                }
                preferences.putString("flash", "done");
                preferences.end();
                Serial.println("credientials Saved");
                ESP.restart();
            }
            break;
          }
        } 
        else {
          currentLine = "";
        }
      } 
      else if (c != '\r') {
        currentLine += c;
      }
    }
  }
  header = "";
  // Close the connection
  client.stop();
  //Serial.println("Client disconnected.");
  //Serial.println("");
}

//////////////////////////  setup funtion
void setup() 
{
    // setup code parameters to run once:
    pinMode(DE_RE_pin, OUTPUT);  // RE & DE pins of rs485
    pinMode(ledpin, OUTPUT);  // led pin
    pinMode(resetPin, INPUT_PULLUP);  // Set the reset pin as input with internal pull-up resistor(default state is high)
    
    digitalWrite(resetPin, HIGH);
    digitalWrite(ledpin, LOW);

    Serial2.begin(19200, SERIAL_8E1, RXD2, TXD2); // uart setting for slave (buadrate, serial, 8bits, even parity, 1 stop bit)
    Serial.begin(9600);

    mqtt_client.setServer(mqtt_broker, mqtt_port); // setting server with mqtt broker and port->intialized
    mqtt_client.setCallback(callback);

    preferences.begin("credentials", false);// "credentials" is the name of the first namespace, false = read/write mode
    preferences_1.begin("data", false);// "data" is the name of the second namespace, false = read/write mode

    String flash_flag = preferences.getString("flash", ""); //reading flag from flash, if ssid, pwd, address in flash memory is empty or not
    // count =  preferences_1.getString("count", "").toInt();
    // Serial.println("count");
    // Serial.println(count);
    //count = count_1.toInt();
    if (flash_flag != "") 
    {
       ssid = preferences.getString("ssid", "");              //ssid
       password = preferences.getString("password", "");      //password               reading from flash memory 
       ip = preferences.getString("staticIP", "");            //IP address
       Add_flash[0] = preferences.getString("Address_1", "");  //voltage address
       Add_flash[1] = preferences.getString("Address_2", "");  //current address
       Add_flash[2] = preferences.getString("Address_3", "");  //pf address
       Add_flash[3] = preferences.getString("Address_4", "");  //kva address
       Add_flash[4] = preferences.getString("Address_5", "");  //kwh address

        // printing addresses after reading from flash
        Serial.println(" ");
       for (int d = 0; d < 5; d++) 
       {
        Serial.println(Add_flash[d]); 
       }

       if(ip != "")
       {
         Serial.println("ip");
         Serial.println(ip);
         convert(ip);  // function call fo string to int variables
         Serial.print("Initialized IP Address: ");
         IPAddress local_IP(a, b, c, d);   //converting int variables into IPAddress 
         Serial.println(local_IP); 
         //  }
  
         //  //////////////////////////    static IP
         //  if(ip != "")
         //  {
         //    IPAddress local_IP(a, b, c, d); 
         if (!WiFi.config(local_IP, gateway, subnet))
         {
           Serial.println("STA Failed to configure");  // setting static ip address
         }
       }
       ////////////////////  wifi connection
       if (ssid != "" && password != "") 
       {
        Serial.println(ssid);
        Serial.println(password);

        WiFi.begin(ssid.c_str(), password.c_str());  // global wifi begin

        while (WiFi.status() != WL_CONNECTED) 
        {
          Serial.println("connecting...");
          delay(2000);
          retry_count++;
          if (retry_count > 10) 
          {
            ESP.restart();
          }
          hard_reset_button();
          periodic_send_5sec(millis()); // transmitting 5 tx frames for every 5 seconds
          save_to_flash(); //saving received data into falsh memory
        }

        if (WiFi.status() == WL_CONNECTED) 
        {
           Serial.println("connected to");
           Serial.println(ssid);
           Serial.println("IP address: ");
           Serial.println(WiFi.localIP());
           server.begin();
           MODE = 1;
           Serial.print("mac");
           Serial.println(WiFi.macAddress());
           ///////////   mqtt
           //connecting to a mqtt broker

           if (mqtt_client.connect(mqtt_client_id.c_str())) // connecting to mqtt broker
            {
                Serial.println(" MQTT broker connected"); //printing connected to mqtt broker
            }
            else
            {
              Serial.println(" MQTT broker not connected");
            }
            // Publish and subscribe
            mqtt_client.publish(topic, "Hi, I'm ESP32 energymeter ^^"); //sending/publishing data to mqtt broker
           // mqtt_client.subscribe(topic);  // subcribing/getting data from mqtt broker of topic[esp32]->intialized
         }
      }
      else 
      { 
      setap();  //setting esp32 into setAP mode
      }
    }
    else 
    { 
       setap();   //setting esp32 into setAP mode
    }

     
      for(int a=0; a<5; a++)
      {
        if(Add_flash[a] != "")
        {
          MHeaders[a] = Add_flash[a].toInt();  //  storing flash address into Mheaders array from string to int datatype
        }
      }
      Maddr = MHeaders[Transmitidx];
      //Transmit(Maddr);
    digitalWrite(ledpin, HIGH);
    PUBLISH = 0;
    prev_time = millis();
}

void loop() 
{  // put your main code here, to run repeatedly:
    
  //start_time = millis();
  if(MODE)
  {
    periodic_send_5sec(millis()); // transmitting 5 tx frames for every 5 seconds
  }
  hard_reset_button();
  wifi_server_code();

  if (Serial2.available() > 0) //   if frame received  
  {
    int rx_count = (numRegs * 2) + 5;  //total no. of bytes to be read from slave = no. of registers of tx + remaining frame
    Serial2.readBytes(RxData, rx_count);
    // for(int x=0; x<rx_count; x++)
    // {
    // Serial.println(RxData[x]);
    // }
    readRegs(RxData, Transmitidx, numRegs);// received byte data, address order[0-5] ,no. of registers
  }

  if(PUBLISH)  //PUBLISH mode set in switch state and counts if change
  {
    publish();  //function call for publish switchs data to broker
  }

  // Check if the ESP is connected to WiFi
  if (WiFi.status() == WL_CONNECTED) 
  {
    if(mqtt_client.connected()) //  retry for mqtt server
    {
      mqtt_client.loop();  // Keep the MQTT connection alive
     // Serial.println("connected to mqtt");
    }  
    else
    {
       //Serial.println("reconnecting to mqtt");
       //reconnect(); //function call for mqtt server if connection lost
    }
  }
  else
  {
    if (MODE) //  retry for wifi in wifi mode
    {
       while (WiFi.status() != WL_CONNECTED) 
       {
          Serial.println("connecting...");
          delay(2000);
          periodic_send_5sec(millis()); // transmitting 5 tx frames for every 5 seconds
          retry_count++;
          if (retry_count > 10) 
          {
             ESP.restart();
          }
       }
    }
  }

}








