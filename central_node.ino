#include <TaskScheduler.h>
#include <Wire.h>
#include <PID_v1.h>
#include <SparkFunSerialGraphicLCD.h>//inculde the Serial Graphic LCD library
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

//prototype function for the tasks.
void PIDcallback();
void EC_callback();
void LCD_callback();
void mqtt_callback();

//instantiate ethernet class
EthernetClient ethClient;
//instantiate scheduler class
Scheduler runner;
//instantiate LCD class
LCD LCD;

Task pid_main(15000, TASK_FOREVER, &PIDcallback);//heat controller loop
Task pid_EC(13000, TASK_FOREVER, &EC_callback);//EC contoller loop
Task LCD_task(5000, TASK_FOREVER, &LCD_callback);// lcd task
Task mqtt_task(20000, TASK_FOREVER, &mqtt_callback);//mqtt task

//Define Variables we'll be connecting to
double Setpoint, Input, Output;
double EC_setpoint, EC_input, EC_output;

//Define the aggressive and conservative Tuning Parameters
double aggKp=4, aggKi=0.2, aggKd=1;
double consKp=1, consKi=0.05, consKd=0.25;
//EC loop tuning parameters. 
double ec_Kp=2, ec_Ki=5, ec_Kd=1;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);
PID my_EC(&EC_input, &EC_output, &EC_setpoint, ec_Kp, ec_Ki, ec_Kd, DIRECT);

//variables for MQTT task

// Update this to either the MAC address found on the sticker on your ethernet shield (newer shields)
// or a different random hexadecimal value (change at least the last four bytes)
byte mac[]    = {0xDE, 0xED, 0xBF, 0xAC, 0xFD, 0xEC };
char macstr[] = "deedbafefeed";
// Note this next value is only used if you intend to test against a local MQTT server
byte localserver[] = {192, 168, 1, 142 };
// Update this value to an appropriate open IP on your local network
byte ip[]     = {192, 168, 1, 152 };

char servername[]="quickstart.messaging.internetofthings.ibmcloud.com";
String clientName = String("d:quickstart:arduino:") + macstr;
String topicName = String("iot-2/evt/status/fmt/json");




// Uncomment this next line and comment out the line after it to test against a local MQTT server
PubSubClient client(localserver, 1883, 0, ethClient);
//PubSubClient client(servername, 1883, 0, ethClient);


//global variables.
int temp;
int ec;
int humidity;
float t[2]={};
//bounded window size for relay PID
int WindowSize = 5000;
unsigned long windowStartTime;


void setup() {
  delay(1200);///wait for the one second spalsh screen before anything is sent to the LCD.
  //init the window start time.
  windowStartTime = millis();

  //setup ethernet
  Ethernet.begin(mac, ip);

  LCD.setHome();//set the cursor back to 0,0.
  LCD.clearScreen();//clear anything that may have been previously printed ot the screen.
  delay(10);
  
  LCD.printStr("Commence cultivation chamber");
  delay(1500);
  Serial.begin(9600);
  Wire.begin(); // join i2c bus (address optional for master)
  runner.init();
  Serial.println("scheduler started");
  runner.addTask(pid_main);
  pid_main.enable();

  Setpoint = 25;
  Serial.println("set point set");
  EC_setpoint = 1;

   //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);
  
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  my_EC.SetMode(AUTOMATIC);
  Serial.println("pid started");
}


// EC pid controller callback function
void EC_callback(){
    EC_input = get_EC();
    myPID.Compute();
    int out = EC_output;
    /************************************************
     * turn the output pin on/off based on pid output
     ************************************************/
    unsigned long now = millis();
    if(now - windowStartTime>WindowSize)
    { //time to shift the Relay Window
      windowStartTime += WindowSize;
    }
    if(Output > now - windowStartTime) {
      Wire.beginTransmission(6); // transmit to device #6
    Wire.write(1);
    Wire.write(out);              // sends one byte, the payload to the actuator node
    Serial.println(out);
    Wire.endTransmission();// stop transmitting
    }
    else {
      Wire.beginTransmission(6); // transmit to device #6
    Wire.write(2);
    Wire.write(out);              // sends one byte, the payload to the actuator node
    Serial.println(out);
    Wire.endTransmission();// stop transmitting
      }
}
//heat controller PID function callback
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
  Wire.write(1);
  Wire.write(payload);              // sends one byte, the payload to the actuator node
  Serial.println(payload);
  Wire.endTransmission();// stop transmitting
}


//call back function for MQTT
void mqtt_callback(){
      char clientStr[34];
      clientName.toCharArray(clientStr,34);
      char topicStr[26];
      topicName.toCharArray(topicStr,26);
      get_temp();
      get_humidity();
      get_EC();
      if (!client.connected()) {
        Serial.print("Trying to connect to: ");
        Serial.println(clientStr);
        client.connect(clientStr);
      }
      if (client.connected() ) {
        String json = buildJson();
        char jsonStr[200];
        json.toCharArray(jsonStr,200);
        boolean pubresult = client.publish(topicStr,jsonStr);
        Serial.print("attempt to send ");
        Serial.println(jsonStr);
        Serial.print("to ");
        Serial.println(topicStr);
        if (pubresult)
          Serial.println("successfully sent");
        else
          Serial.println("unsuccessfully sent");
      }
      delay(5000);
}


//function to build JSON string for MQTT
String buildJson() {
  String data = "{";
  data+="\n";
  data+= "\"d\": {";
  data+="\n";
  data+="\"myName\": \"Plant cultivation LOG\",";
  data+="\n";
  data+="\"temperature (C)\": ";
  data+=(int)temp;
  data+= ",";
  data+="\n";
  data+="\"Humidity \": ";
  data+=(int)humidity;
  data+= ",";
  data+="\n";
  data+="\"EC\": ";
  data+=(int)ec;
  data+="\n";
  data+="}";
  data+="\n";
  data+="}";
  return data;
}

void LCD_callback(){
    LCD.printStr("Temp = ");
    LCD.printNum(get_temp());
    LCD.printStr("Humidity = ");
    LCD.printNum(get_humidity());
    LCD.printStr("EC = ");
    LCD.printNum(get_EC());
  
}

int get_temp(){
      Wire.requestFrom(8, 3);    // request 1 bytes from slave device #8
      int i = 0;
      while (Wire.available()) { 
        t[i] = Wire.read(); // every character that arrives it put in order in the empty array "t"
        i=i+1;
      }
    
    float temp=t[0];
    return temp;
}

//returns humidity
int get_humidity(){
    Wire.requestFrom(8, 3);    // request 1 bytes from slave device #8
    int i = 0;
    while (Wire.available()) { 
      t[i] = Wire.read(); // every character that arrives it put in order in the empty array "t"
      i=i+1;
    }
      float humidity=t[1];
      return humidity; 
}

//returns EC 
int get_EC(){
    Wire.requestFrom(8,3);
    int i = 0;
    while (Wire.available()) { 
      t[i] = Wire.read(); // every character that arrives it put in order in the empty array "t"
      i=i+1;
    }
    float ec=t[2];
    return ec;
}

void loop() {
  runner.execute();
}
  
