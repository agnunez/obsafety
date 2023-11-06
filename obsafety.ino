/******************************************************
 * obsafety
 * 
 * Driver for an Observatory Safety conditions device
 * Based on ESP32, MLX90614 and BME280 hardware 
 * connected by I2C bus
 * Using wifi with REST api
 * (c) agnuca 2023
 * 
 *****************************************************/
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#define DEBUG true
#define SDApin 22     // Pins at SDA=22 SCL=21. Change them as required
#define SCLpin 21
Adafruit_BME280 bme;  // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

unsigned long measureDelay = 5000;   // Sensors read cycle in ms. Always greater than 3000
unsigned long lastTimeRan;

WebServer server(80);

float temperature;
float humidity;
float pressure;
float tempamb;
float tempobj;

// limits

float limit_tamb = 0;
float limit_tsky = -15;
float limit_humid = 85;

StaticJsonDocument<1024> jsonDocument;  // REST API server configuration

char buffer[1024];

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  limit_tsky = jsonDocument["limit_tsky"];
  // Respond to the client
  server.send(200, "application/json", "{}");
}

void createJson(char *name, float value, char *unit) {  
  jsonDocument.clear();
  jsonDocument["name"] = name;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);  
}
 
void addJsonObject(char *name, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["name"] = name;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void getValues() {
  if(DEBUG) Serial.println("Get all values");
  jsonDocument.clear(); // Clear json buffer
  addJsonObject("temperature", temperature, "°C");
  addJsonObject("humidity", humidity, "%");
  addJsonObject("pressure", pressure, "hPa");
  addJsonObject("tempamb", tempamb, "°C");
  addJsonObject("tempobj", tempobj, "°C");

  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

const char index_html[] PROGMEM = R"rawliteral(
<html><body>
<h2>Obsafety Web</h2>
Please use:
<ul>
<li>GET ip/json
<li>POST ip/set
</ul></body></html>
)rawliteral";

void homePage() {
  if(DEBUG) Serial.println("home page");
  //server.send(200, "text/html", "<html><body><h2>Obsafety Web</h2>Please use:<ul><li>GET ip/json<li>POST ip/set</ul></body></html>");
  server.send(200, "text/html", index_html);
}

void handle_NotFound(){
  server.send(404, "text/plain", "Page Not found");
}

void setupApi() {   // REST API
  server.on("/", homePage);
  server.on("/json", getValues);
  server.on("/set", HTTP_POST, handlePost);
  server.onNotFound(handle_NotFound);
  server.begin();
}


void readSensors(){
  temperature = bme.readTemperature();
  humidity    = bme.readHumidity();
  pressure    = bme.readPressure()/ 100.0F;
  tempamb     = mlx.readAmbientTempC();
  tempobj     = mlx.readObjectTempC();
}

void setup() {
  Serial.begin(115200); 
  if(DEBUG) Serial.println("Init");  
  delay(1500);                 // wait for Serial Monitor
  Wire.end();                  // Set I2C pinout
  Wire.setPins(SDApin,SCLpin);  
  Wire.begin();
  mlx.begin();                 // Initialize sensors
  bme.begin(0x76);
  WiFiManager wm;              // Wifi initiate in AP if no STA mode credential exist
  //wm.resetSettings();        // clear credentials for development testing
  bool res;
  res = wm.autoConnect("obsafetyAP");
  if(res) Serial.println("connected!"); //if you get here you have connected to the WiFi    
  setupApi();                  // start REST API
}

void loop() {
  server.handleClient();
  if (millis() > lastTimeRan + measureDelay)  {   // read every measureDelay without blocking Webserver
    readSensors();
    if(DEBUG) printBME();
    lastTimeRan = millis();
  }
}

void printMLX(){
  Serial.print("MLX90614 sensor\n"); 
  Serial.print("===============\n"); 
  Serial.print("Ambient Temperature= "); 
  Serial.print(mlx.readAmbientTempC()); Serial.println(" °C");
  Serial.print("Object Temperature = "); 
  Serial.print(mlx.readObjectTempC()); Serial.println(" °C"); 
  Serial.println();
}

void printBME() {
  Serial.print("BME280 sensor\n"); 
  Serial.print("=============\n"); 
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");
    Serial.print("Pressure = ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");
    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(1013.25));
    Serial.println(" m");
    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");
    Serial.println();
}
