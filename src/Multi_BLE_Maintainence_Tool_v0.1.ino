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
#include <DHT.h>
#include <Adafruit_Sensor.h>

//Set Const
const int oneWireBus = 4;
const int DHTpin = 2;
const int baroPreassure = 1013;
const char* ssid     = "Multi Maintainence Tool v0.1";
const char* password = "123456789";
const char* DHTtype = "DHT11";



// Variable to store the HTTP request
String header;

//Set oneWire lib
OneWire oneWire(oneWireBus);

//Set Dallas lib
DallasTemperature sensors(&oneWire);

//Set DHT lib
DHT dht(DHTpin, DHT11);

//Init Simple Meteo Calc
SimpleMeteoCalc mc;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 250;

//Set var for temp readings and calc
float tempReadingC = sensors.getTempCByIndex(0);
float humReadingRh = dht.readHumidity();
float dhtTemp = dht.readTemperature();


//Collect sensor data and parse to JSON
String getSensorReadings(){
  climateCalculator();
  readings["temperature"] = String(sensors.getTempCByIndex(0));
  readings["humidity"] = String(mc.getHumidity());
  readings["temperaturedewpoint"] = String(mc.getDewPoint());
  readings["humidityabsolute"] =  String(mc.getHumidityAbsolute());
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

void climateCalculator() {
  sensors.requestTemperatures();
  dht.readHumidity();
  dht.readTemperature();
  mc.setTemperature(tempReadingC);
  mc.setHumidity(humReadingRh);
  mc.setPressure(baroPreassure);
  mc.calculate();
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

  //set meteo and calculate 
  mc.setTemperature(tempReadingC);
  mc.setHumidity(40.0);
  mc.setPressure(1013);
  mc.calculate();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 0,5 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
    Serial.println(dhtTemp);
  }
}