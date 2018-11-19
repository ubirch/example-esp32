////
//// Created by wowa on 15.11.18.
////
//
//#include "../Grove_Temperature_And_Humidity_Sensor/DHT.h"
//#include "temp_sensor.h"
//
//
//#define DHTPIN GPIO_NUM_4     // what pin we're connected to
//
//// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
////#define DHTTYPE DHT22   // DHT 22  (AM2302)
////#define DHTTYPE DHT21   // DHT 21 (AM2301)
//
//// Connect pin 1 (on the left) of the sensor to +5V
//// Connect pin 2 of the sensor to whatever your DHTPIN is
//// Connect pin 4 (on the right) of the sensor to GROUND
//// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
//
//DHT dht(DHTPIN, DHTTYPE);
//
//void initSensor(void){
//    dht.begin();
//}
//void readTemperature(void){
//    // Reading temperature or humidity takes about 250 milliseconds!
//    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
//    float h = dht.readHumidity();
//    float t = dht.readTemperature();
//
//    // check if returns are valid, if they are NaN (not a number) then something went wrong!
//    if (isnan(t) || isnan(h))
//    {
//        printf("Failed to read from DHT\r\n");
//    }
//    else
//    {
//        printf("Humidity: %f %\t Temperature %f", h, t);
//    }
//
//
//}
