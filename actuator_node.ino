#include <TaskScheduler.h>
#include <Wire.h>

#define LIGHTS_RELAY 8
#define PUMP_RELAY 9
#define ACTUATOR_ADDRESS 6
#define FAN_CONTROL 2


void pumpCallback();
void lightsCallback();
void fanCallback();

bool lights_state = false;
bool pump_state = false;


Task t_pump(4000, TASK_FOREVER, &pumpCallback);
Task t_lights(1000, TASK_FOREVER, &lightsCallback);

Scheduler runner;

void setup() {
  Wire.begin(ACTUATOR_ADDRESS);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output

   pinMode(LIGHTS_RELAY, OUTPUT);       
  pinMode(PUMP_RELAY, OUTPUT);

   runner.init();
  Serial.println("Scheduler started");

  runner.addTask(t_pump);
  runner.addTask(t_lights);

  t_pump.enable();
  t_lights.enable();

  Wire.onReceive(receiveEvent);
}

void loop() {
  //delay(100);
  runner.execute();
}

void pumpCallback() {
  //this function controls diaphragm pump, turning it on/off
  // at a predefined interval
  //Serial.println("test");
      if (pump_state == false) {
        digitalWrite(PUMP_RELAY, HIGH);
      pump_state = true;
    } else {
      pump_state = false;
      digitalWrite(PUMP_RELAY, LOW);

    }
}


void receiveEvent(int howMany) {
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x); 
  analogWrite(FAN_CONTROL, x); 
}

void lightsCallback() {
  //this function controls the LED lights
  //turning it on and off at a predefined interval to mimic sun light
  Serial.println("fdgtg");
      if (lights_state == false) {
        digitalWrite(LIGHTS_RELAY, HIGH);
      lights_state = true;
    } else {
      lights_state = false;
      digitalWrite(LIGHTS_RELAY, LOW);
    }
}
