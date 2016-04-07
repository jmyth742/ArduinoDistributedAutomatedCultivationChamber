#include <TaskScheduler.h>
#include <Wire.h>
#include <PID_v1.h>
#include <SparkFunSerialGraphicLCD.h>//inculde the Serial Graphic LCD library
#include <SoftwareSerial.h>

void PIDcallback();

Task pid_main(4000, TASK_FOREVER, &PIDcallback);

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Define the aggressive and conservative Tuning Parameters
double aggKp=4, aggKi=0.2, aggKd=1;
double consKp=1, consKi=0.05, consKd=0.25;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);


Scheduler runner;
int temp;

LCD LCD;

void setup() {
  delay(1200);///wait for the one second spalsh screen before anything is sent to the LCD.

LCD.setHome();//set the cursor back to 0,0.
LCD.clearScreen();//clear anything that may have been previously printed ot the screen.
delay(10);
  
LCD.printStr("Commence Arduino Demo Mode");
delay(1500);
  Serial.begin(9600);
  Wire.begin(); // join i2c bus (address optional for master)
  runner.init();
  Serial.println("scheduler started");
runner.addTask(pid_main);
  pid_main.enable();

  Setpoint = 0;
  Serial.println("set point set");

  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  Serial.println("pid started");
}

int get_temp(){
  Wire.requestFrom(8, 1);    // request 1 bytes from slave device #8
  temp = Wire.read();// receive a byte as character
  Serial.println("temp is: ");
  Serial.print(temp);         // print the character
return temp;
}


void PIDcallback(){
  
   Input = get_temp();
   Serial.print("input is ");
   Serial.println(Input);

  double gap = abs(Setpoint-Input); //distance away from setpoint
  if (gap < 10)
  {  //we're close to setpoint, use conservative tuning parameters
    Serial.println("im on standard tuning");
    myPID.SetTunings(consKp, consKi, consKd);
  }
  else
  {
     //we're far from setpoint, use aggressive tuning parameters
     Serial.println("aggressive tuning");
     myPID.SetTunings(aggKp, aggKi, aggKd);
  }

  myPID.Compute();
  Serial.println(Output);
  int payload = Output;
  int check = 185;
  Wire.beginTransmission(6); // transmit to device #6
  Wire.write(payload);              // sends one byte
  Serial.println(payload);
  Wire.endTransmission();// stop transmitting
  //analogWrite(PIN_OUTPUT, Output);
}

void loop() {
  runner.execute();
}
  
