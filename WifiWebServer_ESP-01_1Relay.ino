/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server communicates with the relay board via the Serial port
 *    http://server_ip/1/on will turn the relay 1 on
 *    http://server_ip/1/1/off will turn the relay 1 off
 *    http://server_ip/2/on will turn the relay 2 on
 *    http://server_ip/2/off will turn the relay 2 off
 *    There are no Serial.print lines of code
 *    
 *    2020-01-26 Created from SolderingStationOTA
 *    for both 1 and 2-channel ttl serial relay module
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h> // 2019-10-10
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "MyAccessPoint"; 
const char* password = "MySecretPassword"; 
IPAddress subnet(255,255,255,0);
WiFiServer server (8080);

//Hex command to send to serial for close relay
byte relON[]  = {0xA0, 0x01, 0x01, 0xA2};

//Hex command to send to serial for open relay
byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1};

//Hex command to send to serial for close relay
byte rel2ON[]  = {0xA0, 0x02, 0x01, 0xA3};

//Hex command to send to serial for open relay
byte rel2OFF[] = {0xA0, 0x02, 0x00, 0xA2};

//Hex command to send to serial for read relay
byte relINP[] = {0xA0, 0x01, 0x02, 0xA3};
  
void setup () 
{
  delay (10);
  //Serial.begin (115200); // 2-channel relay
  Serial.begin (9600);   // 1-channel serial relay
    
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_OFF); // 2018-10-09 workaround
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // WiFi.config(staticIP, gateway, subnet); // Static IP. Not required for dhcp  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.println("");
  Serial.println("WiFi connected");

  // 2019-10-10
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("WiFiWebServer_ESP-01_1RelayOTA"); // 2010-10-10

  // No authentication by default
  ArduinoOTA.setPassword("drachenzahne"); // 2010-10-10

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // 2018-09-11 Show SDK version
  Serial.println("Expressif SDK Version should be later than 2.6.0");
  Serial.println(ESP.getSdkVersion());
  
  // Start the server
  server.begin();
  delay(50);

  Serial.write (relON, sizeof(relON)); // 2019-10-13 espota.py test
  delay(1000);
  Serial.write (relOFF, sizeof(relOFF)); // 2019-10-13 espota.py test 
}

static int dots = 0;
static int val = 0; // 2018-10-25
static int commas = 0;

void loop() {

  ArduinoOTA.handle(); // 2019-10-10 

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connection failed while in loop()! Rebooting in 5s ...");
    delay(5000);
    ESP.restart();
  }
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if ( ! client ) {
    return;
  }

  //Wait until the client sends some data
  while ( ! client.available () )
  {
    delay (10); // 2019-02-28 reduced from 100
    Serial.print(".");
    if (commas++ > 5) {
      commas = 0;
      Serial.println("Terminating client connecting without reading");
      delay(20);
      client.stop();
      return;
    }  
  }

  Serial.println("new client connect, waiting for data ");
  
  // Read the first line of the request
  String req = client.readStringUntil ('\r');
  client.flush ();
  
  // Match the request
  String s = "";
  // Prepare the response
  if (req.indexOf ("/1/on") != -1)
  {
    Serial.write (relON, sizeof(relON));
    delay(100); 
    int relaystate = Serial.read();
    val = relaystate;
    // val |= 0x01; // if you want feedback see below
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus is ";
    //s += (val)?"on":"off";
    s += val; // 2018-10-25
    s += "</html>\n";
  } else if (req.indexOf ("/1/off") != -1) {
      Serial.write (relOFF, sizeof(relOFF)); 
      delay(100); 
      int relaystate = Serial.read();
      val = relaystate;      
      // val &= 0xfe; // if you want feedback
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus is ";
      //s += (val)?"on":"off";
      s += val; // 2018-10-25
      s += "</html>\n";
  } else if (req.indexOf ("/2/on") != -1) {
      Serial.write (rel2ON, sizeof(rel2ON)); 
      val |= 0x02; // if you want feedback see below
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus is ";
      //s += (val)?"on":"off";
      s += val; // 2018-10-25
      s += "</html>\n";
  } else if (req.indexOf ("/2/off") != -1) {
      Serial.write (rel2OFF, sizeof(rel2OFF)); 
      val &= 0xfd; // if you want feedback
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus is ";
      //s += (val)?"on":"off";
      s += val; // 2018-10-25
      s += "</html>\n";
  } else if (req.indexOf ("/1/read") != -1) {
      Serial.write (relINP, sizeof(relINP)); 
      delay(100); 
      int relaystate = Serial.read();
      val = relaystate;      
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus from read is ";
      //s += (val)?"on":"off";
      s += val; // 2018-10-25
      s += "</html>\n";
  } else if (req.indexOf("/index.html") != -1) {
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nStatus is ";
      //s += (val)?"on":"off";
      s += val; // 2018-10-25
      s += " SDK Version is ";
      s += ESP.getSdkVersion();
      s += "\r\n";
      s += "</html>\n";
  }    
  else {
    Serial.println("invalid request");
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nInvalid request</html>\n";
  }

  // Send the response to the client
  client.print(s);
  client.flush();
  delay (100);
  client.stop(); // 2019-02-24
  Serial.println("Client disonnected");
  delay(10);
}

