/******************************************************
 * obsafety
 * 
 * Driver for an Observatory Safety conditions device
 * Based on ESP32, MLX90614 and BME280 hardware 
 * connected by I2C bus
 * Using wifi with REST api
 * (c) agnuca 2023 
 * 
 * Cold Temperature Correction of Cloud Sensor:
 * https://lunaticoastro.com/aagcw/TechInfo/SkyTemperatureModel.pdf
 * DewPoint formula : 
 * https://iridl.ldeo.columbia.edu/dochelp/QA/Basic/dewpoint.html
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

// temperature calculations

float temperature;
float humidity;
float pressure;
float tempamb;
float tempobj;
float tempsky;
float dewpoint;
float limit_tamb = 0;     // freezing below this
float limit_tsky = -15;   // cloudy above this
float limit_humid = 85;   // risk for electronics above this
float limit_dew = 5;      // risk for optics with temp - dewpoint below this
float time2open = 1200;   // waiting time before open roof after a safety close
float time2close = 120;   // waiting time before close roof with continuos overall safety waring for this

bool  status_tamb, status_tsky, status_humid, status_dew, status_weather, status_roof;

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))          // missing math function sign of number

static float k[] = {0., 33., 0., 4., 100., 100., 0., 0.};  // sky temperature corrections polynomial coefficients

float tsky_calc(float ts, float ta){
  float t67,td = 0;
  if (abs(k[2]/10.-ta) < 1) {
    t67=sgn(k[6])*sgn(ta-k[2]/10.) * abs((k[2]/10. - ta));
  } else {
    t67=k[6]*sgn(ta-k[2]/10.)*(log(abs((k[2]/10-ta)))/log(10.) + k[7]/100);
  }
  td = (k[1]/100.)*(ta-k[2]/10.)+(k[3]/100.)*pow(exp(k[4]/1000.*ta),(k[5]/100.))+t67;
  return (ts-td);
}

float dewpoint_calc(float temp, float humid){
  return (temp -((100-humid)/5.));
}

StaticJsonDocument<1024> jsonDocument;  // REST API server configuration

String json_str = "";

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

void addJsonObject(char *name, float value, char *unit, bool comma) {
  json_str += "{ \"name\": \"";
  json_str += name;
  json_str += "\", \"value\": \"";
  json_str += value;
  json_str += "\", \"unit\": \"";
  json_str += unit;
  json_str += "\" }";
  if(comma) json_str += ", ";  
}

void getValues() {
  if(DEBUG) Serial.println("Get all values");
  json_str ="";
  addJsonObject("Temperature", temperature, "°C", true);
  addJsonObject("Humidity", humidity, "%", true);
  addJsonObject("Pressure", pressure, "hPa", true);
  addJsonObject("MLX temp ", tempamb, "°C", true);
  addJsonObject("IR temp", tempobj, "°C", true);
  addJsonObject("Sky Temp", tempsky, "°C", true);
  addJsonObject("Dew Point", dewpoint, "°C", true);
  addJsonObject("Temperature Safety", status_tamb, "Boolean", true);
  addJsonObject("Cloud Safety", status_tsky, "Boolean", true);
  addJsonObject("Humidity Safety", status_humid, "Boolean", true);
  addJsonObject("Dew Safety", status_dew, "Boolean", true);
  addJsonObject("Weather Overall Safety", status_weather, "Boolean", true);
  addJsonObject("Time to open roof", time2open, "sec", true);
  addJsonObject("Time to close roof", time2close, "sec", false);
  String sensors = "{ \"sensors\": [" + json_str + "] }";
  server.send(200, "application/json", sensors);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
</head>
<body>
    <h1>Observatory Safety Data</h1>
    <table border=1 id="myTable" class="table table-striped table-earning">
        <thead>
          <tr>
            <th>Name</th>
            <th>Value</th>
            <th>Units</th>
          </tr>
        </thead>
        <tbody id="testBody"></tbody>
    </table>
    <script>
        const table = document.getElementById("testBody");
        setInterval(update, 5000);
        function update(){
           console.log("timer event")
            fetch('/json')
                .then((response) => response.json())
                .then((data) => renderData(data.sensors));
        };
        function renderData(datos){
            table.innerHTML="";
            datos.forEach(sensor => {
                let row = table.insertRow();
                let name = row.insertCell(0);
                name.innerHTML = sensor.name;
                let value = row.insertCell(1);
                value.innerHTML = sensor.value;
                let unit = row.insertCell(2);
                unit.innerHTML = sensor.unit;
            });
        }
    </script>
</body>
</html> 
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
  server.enableCORS();  
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
  tempsky     = tsky_calc(tempobj, tempamb);
  dewpoint    = dewpoint_calc(temperature, humidity);
// RULES    status true means SAFE!  false means UNSAFE!
  if (tempamb > limit_tamb){
    status_tamb = true;
  }else{
    status_tamb = false;    
  }
  if (humidity < limit_humid){
    status_humid = true;
  }else{
    status_humid = false;    
  }
  if (tempsky < limit_tsky){
    status_tsky = true;
  }else{
    status_tsky = false;    
  }
  if ((tempamb - dewpoint) > limit_dew){
    status_dew = true;
  }else{
    status_dew = false;    
  }
  if (status_tamb && status_humid && status_tsky){
    status_weather = true;
    time2close = 125;
  }else{
    status_weather = false;
    time2open = 1205;
  }
  time2open -= measureDelay;
  if (time2open < 0) time2open = 0;
  time2close -= measureDelay;
  if (time2close < 0) time2close = 0;
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
    lastTimeRan = millis();
    if (time2open > 0) time2open -= measureDelay;
    if (time2close > 0) time2close -= measureDelay;
  }
}
