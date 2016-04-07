#include <Wire.h>

const int temperaturePin = 0;

#define SENSOR_ADDRESS 8

void setup() {
  Serial.begin(9600);
  Wire.begin(SENSOR_ADDRESS);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
}

void loop() {
  delay(5000);
  Serial.println(get_temp());
}

float get_temp(){
   float voltage, degreesC;
  voltage = getVoltage(temperaturePin);
   degreesC = (((voltage - 0.5) * 100.0) /8);
   return degreesC;
}


float getVoltage(int pin){
  return (analogRead(pin) * 0.004882814);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  int t = get_temp();
  Wire.write(t);  // as expected by master
}
