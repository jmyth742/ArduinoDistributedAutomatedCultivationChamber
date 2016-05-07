#include <Wire.h>
#include "DHT.h"


#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

#define ECPIN 0

//global variables
float ec;
int v;
float current = 0.04;
int temp_max = 45;
int ec_max = 2;
int humid_max = 75;

#define SENSOR_ADDRESS 8

DHT dht(DHTPIN, DHTTYPE);//instance of DHT class instantiated

void setup() {
  Serial.begin(9600);
  Serial.println("begin sensor read");
   dht.begin();
  Wire.begin(SENSOR_ADDRESS);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
}

void loop() {
  delay(5000);
  Serial.println(get_temp());//used for debugging
}

float get_temp(){
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  if(t>temp_max){
    t=45;
  }
   return t;
}

float get_ec(){
  v = analogRead(ECPIN);
  //convert the value to voltage
  v = v*(5.0/1023);// this gives us the current voltage across the probe
  //now we need to get the inverse of the resistance which is siemens
  ec = current/v;
  if(ec>ec_max){
    ec=2;
  }
  return ec;
}

float get_humidity(){
  float h = dht.readHumidity();
  if(h > humid_max){
    h = 75;
  }
  return h;
}



// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  int t = get_temp();
  int h = get_humidity();
  int ec = get_ec();
  Wire.write(t);  // as expected by master
  Wire.write(h);
  Wire.write(ec);
}
