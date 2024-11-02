#include "max6675.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

String header ="";  
WiFiServer server(80);

unsigned long currentTime = millis(); //for client
unsigned long previousTime = 0;
const long timeoutTime = 2000;

const char* ssid_h = "ESP32_temp";  // hotpot name and password  
const char* password_h = "12345";    //


//for sensor one of max6675
int thermoCLK1 = 12;  //clk pin
int thermoCS1 = 14;  //cs pin
int thermoDO1 = 27;  //do pin
MAX6675 thermocouple1(thermoCLK1, thermoCS1, thermoDO1); //begin the sensor 1 with pins

//for sensor 2
int thermoCLK2 = 26; //clk pin
int thermoCS2 = 25;   //cs pin
int thermoDO2 = 33; //do pin
MAX6675 thermocouple2(thermoCLK2, thermoCS2, thermoDO2); //begin the sensor 2 with pins


void setup() {
  Serial.begin(115200);
  Serial.println(" Mutiple MAX6675 with ESP32");
    Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid_h, password_h); //start the setAP
  server.begin(); //start the server 
  Serial.println(WiFi.localIP()); //not required fixed IP 192.168.4.1
}

void loop()
{
  Serial.println("sensor1");
  Serial.println(thermocouple1.readCelsius()); //reading the celcius value of sensor1 
  delay(500);
  Serial.println("sensor2");
  Serial.println(thermocouple2.readCelsius()); //reading the celcius value of sensor2
  server_code(); //function call for server_code function 
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
        char c = client.read(); //
        header += c; //reading char by char into header string
        if(c == '\n')
        {
          if(currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();         
            if (header.indexOf("GET /readdata") >= 0) //created page for printing sensor's values in browser
            {
              client.print(thermocouple1.readCelsius());
              client.print(",");
              client.print(thermocouple2.readCelsius());
              break;
            }
            else if(header.indexOf("GET /restart") >= 0) //created page for restarting the esp32
            {
              ESP.restart();
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









