/*
Rafael Rodrigues
Please connect to 
"ESPTempController", 
"superpass"
*/


#include <FS.h>
#include <ESP8266WiFi.h>
#include "DHT.h"
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>


#define DHTTYPE DHT11   // DHT 11

// Set web server port number to 80
WiFiServer server(80);

// DHT Sensor
const int DHTPin = 4;
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);
static char celsiusTemp[7];
static char humidityTemp[7];
// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";


// Assign output variables to GPIO pins
const int output5 = 5;

void setup() {
  Serial.begin(115200);

  WiFiManager wifiManager;

  // reset all config 
  wifiManager.resetSettings();

/*  
 *   If you want to use a static ip 
  IPAddress _ip = IPAddress(192, 168, 1, 9);
  IPAddress _gw = IPAddress(192, 168, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
*/
  wifiManager.autoConnect("ESPTempController", "superpass");
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  dht.begin();
  // Set outputs to LOW
  digitalWrite(output5, LOW);


  // Connect to Wi-Fi network with SSID and password not used
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  //WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

float Ta = 0;
float Tmax = 0;
float Tmin = 0;
float Tp = 0;
boolean bol = true;
void loop() {

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.

          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
          float h = dht.readHumidity();
          // Read temperature as Celsius (the default)
          float t = dht.readTemperature();
          // Read temperature as Fahrenheit (isFahrenheit = true)
          float f = dht.readTemperature(true);
          // Check if any reads failed and exit early (to try again).
          if (isnan(h) || isnan(t) || isnan(f)) {
            Serial.println("Failed to read from DHT sensor!");
            strcpy(celsiusTemp, "Failed");
            strcpy(humidityTemp, "Failed");
          }
          else {
            // Computes temperature values in Celsius and Humidity
            float hic = dht.computeHeatIndex(t, h, false);
            dtostrf(hic, 6, 2, celsiusTemp);

            float t1 = dht.computeHeatIndex(t, h, false);
            //heater controll
            /*
               actual reading previous reading +1
               digital digitalWrite(output5, LOW);
               actual reading previous reading -1
               digital digitalWrite(output5, HIGH);

            */
            if (bol) {
              Tmax = t1;
              Tmin = t1;
              bol = false;
            }

            if (t1 > Tmax) {
              Tmax = t1;
              Serial.println(" Max-------------------------- ");
            } else if (t1 < Tmin) {
              Tmin = t1;
              Serial.println(" Min--------------------------");
            }

            if (t1 < Tmax - 1) {
              digitalWrite(output5, HIGH);
              Serial.print(" Aquecedor liga ");
            } else if (t1 > Tmin + 1) {
              digitalWrite(output5, LOW);
              Serial.print(" Aquecedor desligar ");
            }







            float hif = dht.computeHeatIndex(f, h);
            dtostrf(h, 6, 2, humidityTemp);
            // You can delete the following Serial.print's, it's just for debugging purposes
             Serial.print("Humidity: ");
              Serial.print(h);
              Serial.print(" %\t Temperature: ");
              Serial.print(t);
              Serial.print(" *C ");
             /* Serial.print(f);
              Serial.print(" *F\t Heat index: ");
              Serial.print(hic);
              Serial.print(" *C ");
              Serial.print(hif);
              Serial.print(" *F");
              Serial.print("Humidity: ");
              Serial.print(h);
              Serial.print(" %\t Temperature: ");
              Serial.print(t);
              Serial.print(" *C ");
              Serial.print(f);
              Serial.print(" *F\t Heat index: ");
              Serial.print(hic);
              Serial.print(" *C ");
              Serial.print(hif);
              Serial.println(" *F");*/
          }
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            //Auto refresh
            client.println("Refresh: 5");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("GPIO 5 on");
              output5State = "on";
              digitalWrite(output5, HIGH);
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("GPIO 5 off");
              output5State = "off";
              digitalWrite(output5, LOW);
            }
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4BB543; border: none; color: white; padding: 16px 40px;");//on green
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #CC0000;}</style></head>");//of red

            // Web Page Heading
            client.println("<body><h1>ESP Temp Controller</h1>");

            // Display current state, and ON/OFF buttons for GPIO 5
            client.println("<p>Heater (Pin D1)   - State " + output5State + "</p>");
            // If the output5State is off, it displays the ON button
            if (output5State == "off") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            /*client.println("<h3>Temperature in Celsius: ");
            client.println(celsiusTemp);
            client.println("*C</h3><h3>Humidity: ");
            client.println(humidityTemp);
            client.println("%</h3><h3>");*/
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
