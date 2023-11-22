# obsafety
 Observatory Safety device

 /**************************
 * obsafety
 *
 * Driver for an Observatory Safety conditions device
 * Based on ESP32
 * (c) agnuca 2023
 *
 */

After installing the software, look for ESP32 access point SSID with cellular phone and, after connection, open browser
at http://192.168.4.1, to browse ESP32 IP configuration. Choose in that page your local Wifi Network SSID and password, and
look at the Arduino IDE serial monitor for new ESP32 ip in your LAN. 

Then, open http://youresp32lanip/ and after 5 seconds, a table with all the data will appear, with a form below to allow 
safety limits configuration. The table will refresh every 5s. Change limits at wish and submit.
 
Simple connection schema usign ESP32 WROOM 32, BME280 breakout board and MLX90614 

