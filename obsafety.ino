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


// Set reference sea-level preassure to calculate Altitud
#define SEALEVELPRESSURE_HPA (1013.25)

// Pins at SDA=22 SCL=21. Change them as required
#define SDApin 22
#define SCLpin 21

Adafruit_BME280 bme; // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

WebServer server(80);

float temperature;
float humidity;
float pressure;
float tempamb;
float tempobj;

// REST API server configuration

StaticJsonDocument<1024> jsonDocument;

char buffer[1024];

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);

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
  Serial.println("Get all values");
  jsonDocument.clear(); // Clear json buffer
  addJsonObject("temperature", temperature, "°C");
  addJsonObject("humidity", humidity, "%");
  addJsonObject("pressure", pressure, "hPa");
  addJsonObject("tempamb", tempamb, "°C");
  addJsonObject("tempobj", tempobj, "°C");

  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void homePage() {
  Serial.println("home page");
  server.send(200, "application/json", "<html><body><h2>Obsafety Web</h2>Please GET from ip/getValues or POST to ip/setValues</body></html>");
}

void setupApi() {   // REST API
  server.on("/", homePage);
  server.on("/json", getValues);
  server.on("/set", HTTP_POST, handlePost);
  server.begin();
}

// Timer for sensor reading cycle

hw_timer_t *Timer0 = NULL;
bool sensors_ready_flag = false;

void IRAM_ATTR Timer0_ISR(){
  sensors_ready_flag = true;
}

void readSensors(){
  temperature = bme.readTemperature();
  humidity    = bme.readHumidity();
  pressure    = bme.readPressure();
  tempamb     = mlx.readAmbientTempC();
  tempobj     = mlx.readObjectTempC(); 

  sensors_ready_flag = false;
}

void setup() {
  Serial.begin(115200); 
  Serial.println("Init");  
  delay(1500);                 // wait for Serial Monitor
  Wire.end();                  // Set I2C pinout
  Wire.setPins(SDApin,SCLpin);  
  Wire.begin();
  mlx.begin();                 // Initialize sensors
  bme.begin(0x76);
  WiFiManager wm;              // Wifi initiate in AP if no STA mode credential exist
  //wm.resetSettings();        // clear credentials for development testing
  bool res;
  res = wm.autoConnect("AutoConnectAP");
  if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
  setupApi();                  // start REST API
  
  // Assign ISR to timer and execute ISR every 1s
/*
  Timer0 = timerBegin(0, 80, true); // pre-scaler 8000, true count up
  timerAttachInterrupt(Timer0, &Timer0_ISR, true);
  timerAlarmWrite(Timer0, 5000000, true);
  timerAlarmEnable(Timer0);
*/
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  //if(sensors_ready_flag == true){
    readSensors();
  //}
  delay(1000);
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
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");
    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");
    Serial.println();
}
