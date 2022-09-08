/*************
 *  2022 Brian Rodriguez
 *  Multi BLE Maintainence Tool v0.1
*************/

//Include libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino_JSON.h>
#include <SimpleMeteoCalc.h>
#include <temperature.h>




//Set Const
const int oneWireBus = 4;
const char* ssid     = "Multi Maintainence Tool v0.1";
const char* password = "123456789";




//Set oneWire and Dallas lib and set var for temp. reading
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
float tempReadingC = sensors.getTempCByIndex(0);

//Set TC class and variables for Temperature Lib
temperatureConverter TC;
volatile float dp;

//set const hum var for testing
const int humReading = 54;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Variable to store the HTTP request
String header;
// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 100;




//Collect sensor data and parse to JSON
String getSensorReadings(){
  sensors.requestTemperatures();

  delay(10);
  readings["temperature"] = String(tempReadingC);
  readings["humidity"] = String(humReading);
  readings["temperaturedewpoint"] = String(dewPoint(tempReadingC, humReading), 2);
  readings["humidityabsolute"] =  String();
  String jsonString = JSON.stringify(readings);
  return jsonString;
}




//mount the SPIFFS filesystem
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}




void setup() {
  //Set Serial Boudrate
  Serial.begin(115200);

  //Init filesystem
  initSPIFFS();

  //set sensors
  sensors.begin();


  //Delay to init
  delay(1500);

  //Init WiFi AP, remove the password parameter for open AP
  //Return IPaddress over serial Monitor
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //server.begin();

  // Set Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });

  //Add and init serverhandler events
  server.addHandler(&events);
  
  // Init and start server
  server.begin();

  //Init Temperature Library dependecies
  TC.setCelsius(tempReadingC);
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 0,5 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
}