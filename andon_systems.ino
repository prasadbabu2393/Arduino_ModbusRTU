#include <HardwareSerial.h>
#include <WiFi.h>
#include <Preferences.h>  // Include Preferences library

Preferences preferences;  // Create a Preferences object

const char* ssid_h = "ESP32_switch";  // hotpot name and password
const char* password_h = "12345";

WiFiServer server(80);
String header;
String network_1, password_1;
String ssid, password;

const int resetPin = 21;     ///// Set your reset button pin
int programState = 0, buttonState;
long buttonMillis = 0;

// Set your Static IP address
IPAddress local_IP(192, 168, 0, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);  ///1 -- 3 REPLACED

IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

int switchState[8] = {0, 0, 0, 0, 0, 0, 0, 0};     // Array to store the states of the switches
int lastSwitchState[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Array to store the last states for comparison
int switchCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};    // Array to store the toggle counts for each switch

const int switchPins[8] = {34, 35, 32, 33, 25, 26, 27, 14};  // Pins for the four switches
int led_pin = 5;   // d5 for led
void setup() 
{
  // Initialize the switch pins as inputs
  for (int i = 0; i < 8; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);  // Using INPUT_PULLUP to avoid needing external resistors
  }

  pinMode(resetPin, INPUT_PULLUP); // Set the reset pin as input with internal pull-up resistor(default state is high)
  digitalWrite(resetPin, HIGH);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  Serial.begin(115200);

  // Initialize Preferences and read stored switch states and counts
  preferences.begin("storage", false);  // same namespace for switchs and network, password

  String flash_flag = preferences.getString("flash", "");  //  reading a flag
  if(flash_flag != "")
  {
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");

    Serial.println(ssid);
    Serial.println(password);

      // Configures static IP address
     if (!WiFi.config(local_IP, gateway, subnet)) {      //, primaryDNS, secondaryDNS
     Serial.println("STA Failed to configure");
      }
  
     WiFi.begin(ssid.c_str(), password.c_str());
     int Counter = 0;
     while(WiFi.status() != WL_CONNECTED && Counter < 10)
     {
       Serial.println("connecting...");
       delay(1000);
       Counter++;
     }

     if (WiFi.status() == WL_CONNECTED) 
      {
        Serial.println("connected");
        Serial.println(ssid);
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        server.begin();
      }
      else
      {
        /////////////////////     set ap
       Serial.print("Setting AP (Access Point)…");
        WiFi.softAP(ssid_h, password_h);
       IPAddress IP = WiFi.softAPIP();
       Serial.print("AP IP address: ");
       Serial.println(IP); 
       server.begin();
     }
  }
  else
  {
    /////////////////////     set ap
      Serial.println("No values saved for ssid or password");
      Serial.print("Setting AP (Access Point)…");
      WiFi.softAP(ssid_h, password_h);
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP); 
      server.begin();
  }

 //preferences.begin("switchStates", true);  // Open Preferences for reading
  for (int i = 0; i < 8; i++) {
    switchState[i] = preferences.getInt(String(i).c_str(), 0);  // Read each switch state, default to 0 if not found
    switchCount[i] = preferences.getInt(("count" + String(i)).c_str(), 0);  // Read each switch count, default to 0 if not found
  }
  
  // Print the loaded switch states and counts
  Serial.println("Loaded Switch States and Counts:");
  for (int i = 0; i < 8; i++) 
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
  digitalWrite(led_pin, HIGH);
}

// void debounce(int pin)
// {
//     unsigned long currentMillis = millis();
//      buttonState = digitalRead(pin);
//     if (buttonState == LOW && programState == 0) 
//     {
//        buttonMillis = currentMillis;
//        programState = 1;
//     }
//     else if (programState == 1 && buttonState == HIGH) 
//     {
//         programState = 0; //reset
//     }
//     if(((currentMillis - buttonMillis) > 3000) && programState == 1) 
//     {
//        programState = 0;
//         //ledMillis = currentMillis;
//     }
// }

void loop() 
{
  //////////// reset button function 
 unsigned long currentMillis = millis();
     buttonState = digitalRead(resetPin);
    if (buttonState == LOW && programState == 0) 
    {
       buttonMillis = currentMillis;
       programState = 1;
    }
    else if (programState == 1 && buttonState == HIGH) 
    {
        programState = 0; //reset
    }
    if(((currentMillis - buttonMillis) > 3000) && programState == 1) 
    {
       programState = 0;
        //ledMillis = currentMillis;
      Serial.println(" reseting and clearing credencials");
      preferences.clear();
      preferences.end();
      esp_restart();
    }

  ////////////  switch states function 
  // Check the state of each switch
  for (int i = 0; i < 8; i++)
  {
    int reading = digitalRead(switchPins[i]);
     switchState[i] = reading;
    // If the switch was just pressed (transition from HIGH to LOW)
    if (reading == LOW && lastSwitchState[i] == HIGH) 
    {   
      // Increment the switch count
      switchCount[i]++;
      
      // Store the new state and count in flash
     // preferences.begin("switchStates", false);  // Open namespace in read-write mode
      preferences.putInt(String(i).c_str(), switchState[i]);  // Store each switch state
      preferences.putInt(("count" + String(i)).c_str(), switchCount[i]);  // Store each switch count
      //preferences.end();  // Close Preferences

      // Print the new state and count
      Serial.println("Updated Switch States and Counts:");
      for (int j = 0; j < 8; j++) {
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
              for (int i = 0; i < 8; i++) 
              {
                client.print(switchState[i]);
                if(i<8) client.print(",");
                client.println(switchCount[i]);
                if(i<7) client.print(",");
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
                  break;
            }
            else if(header.indexOf("GET /login") >= 0)
            {
              Serial.println("Logging in");
            
              // Display the HTML web page
              client.println("<!DOCTYPE html>");
              client.println("<html>");
              client.println("<body>");
              client.println("<form action=\"/action_page.php\">");
              client.println("<label for=\"ssid\">ssid: </label>");
              client.println(" <input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>");
              client.println("<label for=\"password\">Password: </label>");
              client.println("<input type=\"text\" id=\"password\" name=\"password\"><br><br>");
              client.println("<input type=\"submit\" value=\"Submit\">");
              client.println("</form>");
              client.println("<p>Enter ssid and Password and press submit</p>");
              client.println("</body></html>");
              client.println();
            }
            else if(header.indexOf("GET /action_page.php") >= 0) 
            {
              int networkStart = header.indexOf("ssid=") + 5;
              int networkEnd = header.indexOf("&", networkStart);
              network_1 = header.substring(networkStart, networkEnd);
              int passwordStart =header.indexOf("password=") + 9;
              int passwordEnd = header.indexOf(" ", passwordStart);
              password_1 = header.substring(passwordStart, passwordEnd);
              // Break out of the while loop
              Serial.println("network");
              Serial.println(network_1);
              Serial.println("password");
              Serial.println(password_1);

                if(network_1 == "" && password_1 == "")
                {
                  Serial.println("Empty ssid password");
                }
                else 
                {
                    if(network_1 != "") {
                      preferences.putString("ssid", network_1); 
                    }
                    if(password_1 != "") {
                     preferences.putString("password", password_1);
                    }
                    preferences.putString("flash", "done");
                    delay(100); 
                    Serial.println("credientials Saved");
                    ESP.restart();
                }
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
