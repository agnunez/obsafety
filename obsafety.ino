/**************************
 * obsafety
 * 
 * Driver for an Observatory Safety conditions device
 * Based on ESP32
 * (c) agnuca 2023
 * 
 */
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(115200); //Se inicia el monitor serie a 115200 baudios
  Serial.println("Init");  
  Wire.end();
  Wire.setPins(22,21);  // Pins at SDA=22 SCL=21. Change as required
  Wire.begin();
  mlx.begin();
  bme.begin(0x76);
}

void loop() {
  // put your main code here, to run repeatedly:
  printMLX();
  printBME();
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
