# 1 "C:\\Users\\BrianR\\AppData\\Local\\Temp\\tmptj46f2bn"
#include <Arduino.h>
# 1 "G:/Mijn Drive/01. Media/11. Development/01. Arduino/01. ESP32/Multi BLE Maintainence Tool v0.1/src/Multi_BLE_Maintainence_Tool_v0.1.ino"






#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino_JSON.h>
#include <SimpleMeteoCalc.h>


const int oneWireBus = 4;
const char* ssid = "Multi Maintainence Tool v0.1";
const char* password = "123456789";


String header;


OneWire oneWire(oneWireBus);


DallasTemperature sensors(&oneWire);


SimpleMeteoCalc mc;

AsyncWebServer server(80);


AsyncEventSource events("/events");


JSONVar readings;


unsigned long lastTime = 0;
unsigned long timerDelay = 10000;
String getSensorReadings();
void initSPIFFS();
void setup();
void loop();
#line 47 "G:/Mijn Drive/01. Media/11. Development/01. Arduino/01. ESP32/Multi BLE Maintainence Tool v0.1/src/Multi_BLE_Maintainence_Tool_v0.1.ino"
String getSensorReadings(){
  sensors.requestTemperatures();
  float tempReadingC = sensors.getTempCByIndex(0);
  mc.setTemperature(tempReadingC);
  mc.setHumidity(40.0);
  mc.setPressure(1017.27);
  mc.setUserAltitude(15.0);
  delay(50);
  mc.calculate();
  readings["temperature"] = String(sensors.getTempCByIndex(0));
  readings["temperature-dewpoint"] = String(mc.getDewPoint());
  readings["humidity-absolute"] = String(mc.getHumidityAbsolute());
  String jsonString = JSON.stringify(readings);
  return jsonString;
}


void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

void setup() {

  Serial.begin(115200);


  initSPIFFS();


  sensors.begin();



  delay(1500);



  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);



  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");


  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }


    client->send("hello!", NULL, millis(), 10000);
  });


  server.addHandler(&events);




  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {

    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }
  float t = mc.getTemperature();
  float h = mc.getHumidity();
  float p = mc.getPressure();
  float a = mc.getUserAltitude();
  float td = mc.getDewPoint();
  float ha = mc.getHumidityAbsolute();
  Serial.println();
  Serial.println();
  Serial.print(F("INPUT:\t\tt="));
  Serial.println();
  Serial.print(t, 3);

  Serial.print(F(" *C\th="));
  Serial.println();
  Serial.print(h, 3);

  Serial.print(F(" mmHg\nTd="));
  Serial.println();
  Serial.print(td, 3);

  Serial.print(F(" m\nHa="));
  Serial.println();
  Serial.print(ha, 3);
  delay(2000);

}