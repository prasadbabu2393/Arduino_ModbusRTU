#include <HardwareSerial.h>
#include "stdint.h"
#include <WiFi.h>
#include <Preferences.h>  // Include Preferences library

const char* ssid = "TP-Link_F237";
const char* pwd = "42191544";
//  const char* ssid = "NagaKumarN";
//  const char* pwd = "nattnatt";

WiFiServer server(80);
String header;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

int switchState[4] = {0, 0, 0, 0};  // Array to store the states of the switches
int lastSwitchState[4] = {0, 0, 0, 0};  // Array to store the last states for comparison
int switchCount[4] = {0, 0, 0, 0};  // Array to store the toggle counts for each switch

const int switchPins[4] = {34, 35, 32, 33};  // Pins for the four switches

Preferences preferences;  // Create a Preferences object

void setup() 
{
  // Initialize the switch pins as inputs
  for (int i = 0; i < 4; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);  // Using INPUT_PULLUP to avoid needing external resistors
  }

  Serial.begin(115200);
  WiFi.begin(ssid, pwd);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("connecting...");
    delay(1000);
  }
  Serial.println("connected");
  Serial.println(ssid);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // Initialize Preferences and read stored switch states and counts
  preferences.begin("switchStates", true);  // Open namespace in read-only mode
 // preferences.clear();
  for (int i = 0; i < 4; i++) {
    switchState[i] = preferences.getInt(String(i).c_str(), 0);  // Read each switch state, default to 0 if not found
    switchCount[i] = preferences.getInt(("count" + String(i)).c_str(), 0);  // Read each switch count, default to 0 if not found
  }
  preferences.end();  // Close Preferences
  
  // Print the loaded switch states and counts
  Serial.println("Loaded Switch States and Counts:");
  for (int i = 0; i < 4; i++) 
  {
    if((String)switchState[i] != "")
    {
    Serial.print("Switch ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(switchState[i]);
    Serial.print(", Count");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(switchCount[i]);
    }
  }
  Serial.println();
}

void loop() 
{
  // Check the state of each switch
  for (int i = 0; i < 4; i++)
  {
    int reading = digitalRead(switchPins[i]);
    
    // If the switch was just pressed (transition from HIGH to LOW)
    if (reading == LOW && lastSwitchState[i] == HIGH) 
    {
      // Toggle the switch state
      switchState[i] = !switchState[i];
      
      // Increment the switch count
      switchCount[i]++;
      
      // Store the new state and count in flash
      preferences.begin("switchStates", false);  // Open namespace in read-write mode
      preferences.putInt(String(i).c_str(), switchState[i]);  // Store each switch state
      preferences.putInt(("count" + String(i)).c_str(), switchCount[i]);  // Store each switch count
      preferences.end();  // Close Preferences

      // Print the new state and count
      Serial.println("Updated Switch States and Counts:");
      for (int j = 0; j < 4; j++) {
        Serial.print("Switch ");
        Serial.print(j + 1);
        Serial.print(": ");
        Serial.print(switchState[j]);
        Serial.print(", Count");
        Serial.print(j + 1);
        Serial.print(": ");
        Serial.println(switchCount[j]);
      }
      Serial.println();
    }
    
    // Update last switch state
    lastSwitchState[i] = reading;
  }
 
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
              for (int i = 0; i < 4; i++) 
              {
                client.print(switchState[i]);
                if(i<4) client.print(",");
                client.println(switchCount[i]);
                if(i<3) client.print(",");
              }
               client.print("}");

                             //client.print(switchState[0]);
              // client.println("Switch States and Counts:");
              //  for (int i = 0; i < 4; i++) 
              //  {
              //   client.print("Switch ");
              //   client.print(i + 1);
              //   client.print(": ");
              //   client.print(switchState[i]);
              //   client.print(", Count");
              //   client.print(i + 1);
              //   client.print(": ");
              //   client.println(switchCount[i]);
              //  }
            }
            break;
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
