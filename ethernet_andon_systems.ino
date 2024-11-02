
#include <WiFi.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Preferences.h>

Preferences preferences;
const char* ssid_h = "ESP32_switch";  // hotpot name and password  ACT102646054057-5g   16761789
const char* password_h = "12345";     // 192.168.0.111  -5g   // 192.168.0.114

const char *mqtt_broker = "192.168.0.193";
const char *topic = "esp32";
const int mqtt_port = 1883;
String mqtt_client_id = "esp32";

// Set the MAC address and IP address for your Ethernet shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ethernet_ip(192, 168, 0, 177); // Set an IP address in your network range

// Create an EthernetServer object
EthernetServer server(80); // Port 80 for HTTP
EthernetClient client;
PubSubClient mqtt_client(client);
// Pin definitions for SPI
#define W5500_CS 5 // Chip Select pin for W5500


//IPAddress local_IP(192, 168, 0, 114);  // 192.168.0.114 // Set your Static IP address
int a = 192, b = 168, c = 0, d = 114;
IPAddress flash_ip;
IPAddress gateway(192, 168, 0, 1);  ///1 -- 3 REPLACED  // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional
   
String header, publish_data = "";    
String network_1, password_1, client_ip, ip;
String ssid, password;
bool MODE, PUBLISH;

int programState = 0, buttonState;
long buttonMillis = 0;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

int switchState[8] = {0, 0, 0, 0, 0, 0, 0, 0};     // Array to store the states of the switches
int lastSwitchState[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Array to store the last states for comparison
int switchCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};    // Array to store the toggle counts for each switch
const int switchPins[8] = {34, 35, 32, 33, 25, 26, 27, 14};  // Pins for the four switches
const int resetPin = 21;     // Set your reset button pin
int led_pin = 5, retry_count = 0;;   // d5 for led

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
       digitalWrite(led_pin,!digitalRead(led_pin));
}


// checking switch States and counts function 
void switch_state_count()
{
      for (int i = 0; i < 8; i++)
       { 
           switchState[i] = digitalRead(switchPins[i]);  // reading the pin in esp32
           if (switchState[i] == LOW && lastSwitchState[i] == HIGH) // If the switch was just pressed (transition from HIGH to LOW)
           {   
             switchCount[i]++;  //Increment the switch count
             preferences.putInt(("count" + String(i)).c_str(), switchCount[i]); // storing into the flash memory
           }
           if(lastSwitchState[i] != switchState[i])  // if any st=witch change or update then pulishing 
           {
             PUBLISH = 1;    //setting mode for publishing
           }
            // Update last switch state
           lastSwitchState[i] = switchState[i];   //storing last state in lastSwitchState variable
       }
}

//this fun is for send the switch states and counts to mqtt broker
void publish()
{
  
  Serial.println("In publish:");
  publish_data = "";
  for(int j=0; j<8; j++)
  {
   //  Serial.println("count" + String(j) +":"); //// printing counts  
   //  Serial.println(switchCount[j]);
   publish_data += (String)switchState[j] + "," + (String)switchCount[j] + ","; //string concatinating of switchs states and counts 
  }
   if (mqtt_client.connect(mqtt_client_id.c_str())) {
            Serial.println(" MQTT broker connected");
        
  mqtt_client.publish(topic, publish_data.c_str());  //sending switchs data to mqtt  broker
  PUBLISH = 0;
   }
}

//this fun is for mqtt server retry connection to broker when connection lost after connected to broker
void reconnect()
{
     while (!mqtt_client.connected()) 
       {
          //String mqtt_client_id = "esp32";
          if (mqtt_client.connect(mqtt_client_id.c_str())) 
          {
            Serial.println("MQTT broker connected");
          } 
          else 
          {
            Serial.print("failed with state ");
            Serial.print(mqtt_client.state());
          }
          hard_reset_button();
          switch_state_count(); 
          server_code();
        }
}



//////// string IP address to int a,b,c,d variables function
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

// this function replaces special chars in password or ssid strings etc which are taking in client
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


void hard_reset_button()
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
    if(((currentMillis - buttonMillis) > 3000) && programState == 1) // 3000 means 3sec holding reset button 
    {
       programState = 0;
        //ledMillis = currentMillis;
      Serial.println(" reseting and clearing credencials");
      preferences.clear();
      preferences.end();
      esp_restart();
    }
}


void server_code()
{
///////////  ethernet server code
   client = server.available(); 
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
             // client.println(cyclestart);
              client.print("}");
              break;
            }
            // else if(header.indexOf("GET /login") >= 0)
            // {
            //   Serial.println("Logging in");
            
            //   // Display the HTML web page
            //   client.println("<!DOCTYPE html>");
            //   client.println("<html>");
            //   client.println("<body>");
            //   client.println("<form action=\"/action_page.php\">");

            //   ////////////////////  ssid and paasword
            //   client.println("<label for=\"ssid\">ssid: </label>");
            //   client.println(" <input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>");
            //   client.println("<label for=\"password\">Password: </label>");
            //   client.println("<input type=\"text\" id=\"password\" name=\"password\"><br><br>");
              
            //   ////////////////////  static IP
            //   client.println("<p> enter IP address like eg: 192.168.0.1 </p>");
            //   client.println("\n");
            //   client.println("<label for=\"static_IP\">static IP : </label>");
            //   client.println("<input type=\"text\" id=\"static_IP\" name=\"static_IP\"><br><br>");

            //   ////////////////////  submit button
            //   client.println("<input type=\"submit\" value=\"Submit\">");
            //   client.println("</form>");
            //      //////////////   script function
            //   client.println("<script>");
            //   client.println("function encodeFormData() {");
            //   client.println("const form = document.forms[0];");
            //   client.println("form.ssid.value = encodeURIComponent(form.ssid.value);");
            //   client.println("form.password.value = encodeURIComponent(form.password.value);");
            //   client.println("form.static_IP.value = encodeURIComponent(form.static_IP.value);");
            //   client.println("form.voltage.value = encodeURIComponent(form.voltage.value);");
            //   client.println("form.current.value = encodeURIComponent(form.current.value);");
            //   client.println("form.pf.value = encodeURIComponent(form.pf.value);");
            //   client.println("form.kva.value = encodeURIComponent(form.kva.value);");
            //   client.println("form.kwh.value = encodeURIComponent(form.kwh.value);");
            //   client.println("}");
            //   client.println("</script>");
            //   //////////////
            //   client.println("<p>Enter ssid and Password and press submit</p>");
            //   client.println("</body></html>");
            //   client.println();
            // }
            // else if(header.indexOf("GET /action_page.php") >= 0) 
            // {
            //   int networkStart = header.indexOf("ssid=") + 5; //seperating ssid string from header string
            //   int networkEnd = header.indexOf("&", networkStart);
            //   network_1 = header.substring(networkStart, networkEnd);
            //   network_1 = urlDecode(network_1);  // Decode SSID
              
            //   int passwordStart =header.indexOf("password=") + 9; //seperating password string from header string
            //   int passwordEnd = header.indexOf("&", passwordStart);
            //   password_1 = header.substring(passwordStart, passwordEnd);
            //   password_1 = urlDecode(password_1);  // Decode password
              
            //   int ipStart = header.indexOf("static_IP=") + 10;//seperating ip address string from header string
            //   int ipEnd = header.indexOf(" ", ipStart);
            //   client_ip = header.substring(ipStart, ipEnd);
            //   // Break out of the while loop
            //   Serial.println("network");
            //   Serial.println(network_1);
            //   Serial.println("password");
            //   Serial.println(password_1);
            //   Serial.println("client_iP");
            //   Serial.println(client_ip);

            //         if(network_1 != "") {
            //           preferences.putString("ssid", network_1); //storing ssid with name ssid into flash memory
            //         }
            //         if(password_1 != "") {
            //          preferences.putString("password", password_1); //storing password with name password into flash memory
            //         }
            //         if(client_ip != "")
            //         {
            //           preferences.putString("staticIP", client_ip); //  storing ip address string with name static_IP into flash memory
            //         }
            //         preferences.putString("flash", "done");
            //         delay(100); 
            //         Serial.println("credientials Saved");
            //         ESP.restart();
            // }
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




void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void setup() 
{
   // Initialize the switch pins as inputs
  for (int i = 0; i < 8; i++) 
  {
    pinMode(switchPins[i], INPUT);  // Using INPUT_PULLUP to avoid needing external resistors
  }

  pinMode(resetPin, INPUT_PULLUP); // Set the reset pin as input with internal pull-up resistor(default state is high)
  digitalWrite(resetPin, HIGH);  
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  Serial.begin(115200);
  
       // Initialize the SPI bus with custom pins
      SPI.begin(18, 19, 23, W5500_CS); // SCK=18, MISO=19, MOSI=23, CS=5
  
       // Start the Ethernet connection and server
      Ethernet.init(W5500_CS); // Initialize Ethernet with CS pin
      Ethernet.begin(mac, ethernet_ip);

      // Give the Ethernet module a second to initialize
      delay(1000);
      // Start listening for clients
      server.begin();
      Serial.print("Server is at ");
      Serial.println(Ethernet.localIP());  // Print IP address to Serial Monitor

      if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
      Serial.print("Ethernet not connected, IP address: ");
      Serial.println(Ethernet.localIP());
      } 

      //connecting to a mqtt broker
      mqtt_client.setServer(mqtt_broker, mqtt_port);
      mqtt_client.setCallback(callback);

      while (!mqtt_client.connected()) 
      {
        String mqtt_client_id = "esp32";
        //client_id += String(WiFi.macAddress());
        //Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (mqtt_client.connect(mqtt_client_id.c_str())) {
            Serial.println(" MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(mqtt_client.state());
            delay(2000);
        }
      }

      // Publish and subscribe
      mqtt_client.publish(topic, "Hi, I'm ESP32 ^^");
      mqtt_client.subscribe(topic);

      Serial.println("loaded counts");  //taking from the flash memory
      for (int i = 0; i < 8; i++) 
      {
        //switchState[i] = preferences.getInt(String(i).c_str(), 0);  // Read each switch state, default to 0 if not found
        switchCount[i] = preferences.getInt(("count" + String(i)).c_str(), 0);  // Read each switch count, default to 0 if not found
        // Serial.println("count" + String(i+1) +":");
        // Serial.println(switchCount[i]);
      }
      digitalWrite(led_pin, HIGH);  //led ON
      PUBLISH = 0;
 }

void loop() 
{
      mqtt_client.loop();//mqtt loop

      hard_reset_button(); //hard reset function call
  switch_state_count();  //function call for switch states and counts 

 if(PUBLISH)  //PUBLISH mode set in switch state and counts if change
  {
    publish();  //function call for publish switchs data to broker
  }

 if(!mqtt_client.connected()) //  retry for mqtt server
  {
    //reconnect(); //function call for mqtt server if connection lost
  } 


server_code();
  // // Listen for incoming clients
  // client = server.available();

  // if (client) {
  //   Serial.println("New client connected");

  //   boolean currentLineIsBlank = true;
  //   while (client.connected()) {
  //     if (client.available()) {
  //       char c = client.read();
  //       Serial.write(c); // Print the incoming request to the Serial Monitor

  //       // If you get a newline character and the line is blank, send a response
  //       if (c == '\n' && currentLineIsBlank) {
  //         // Send a standard HTTP response header
  //         client.println("HTTP/1.1 200 OK");
  //         client.println("Content-Type: text/html");
  //         client.println("Connection: close");
  //         client.println();

  //         // Send a simple HTML message
  //         client.println("<!DOCTYPE HTML>");
  //         client.println("<html>");
  //         client.println("<h1>Hello, World!</h1>");
  //         client.println("</html>");
          
  //         break;
  //       }

  //       // If you get a newline character, set currentLineIsBlank to true, else false
  //       if (c == '\n') {
  //         currentLineIsBlank = true;
  //       } else if (c != '\r') {
  //         currentLineIsBlank = false;
  //       }
  //     }
  //   }

  //   // Give the client time to receive the data
  //   delay(1);

  //   // Close the connection
  //   client.stop();
  //   Serial.println("Client disconnected");
  // }
}
