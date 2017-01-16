#include <ZumoMotors.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>
#include <NewPing.h>

#define LED_PIN 13    // Pin connected to Arduino LED
#define TRIGGER_PIN  12  //Arduino pin connected to the trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin connected to the echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 30  // Maximum distance we want to ping for (in centimeters).

//I found this threshold useful with the sensors. Anything over 400 it reads a line has been hit.
#define QTR_THRESHOLD  400 //Microseconds

#define SPEED 100 //Speed for the Zumo
#define DURATION  300 //Microseconds

//A struct data structure to hold the path that the Zumo has taken when going around the Maze.
//Originally wanted to use a string but this is much easier to pull the information from.
struct Node {
  char type; //This is the type of action the Zumo has taken, for example: 'r' for room, 't' for turn
  char dir; //This variable records the direction of the turn/room i.e 'l' or 'r'
  bool item; //Bool to hold whether an item has been found.
  int room_no; //Room Number is used as an incrementation counter.
};

//Code taken from the ZumoMoters.H
ZumoMotors motors;
byte c; // Byte to hold the Serial command byte 

//Code for the 'W' 'A' 'S' 'D' keys to move the Zumo. 
//Found from the internet
long time_since, t, s, time_move;
bool p;

//Counters to determine which side the room is on. The higher the counter the more likely it's that direction.
int turn_count_l, turn_count_r;
//Initalising room_count to 1 so that the first room incrementer starts at 1.
int room_count = 1; 

//Code from ZumoReflectanceSensor
//0-5 Sensors in an array.
#define NUM_SENSORS 6
unsigned int sensor_values[NUM_SENSORS];
unsigned int position;

//Interger to switch on so that different modes can be accessed.
int mode;

//Array of structs holding the path the Zumo has taken. Allows easy iteration back to retrace steps.
Node path[100];
unsigned int path_length = 0; // the length of the path

//Code taken from ZumoReflectance library
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);

//Code taken from the NewPing library to set up the pins and the distance of the ping in CM.
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
char curr; // current node in the path

void setup()
{
  //Initialising the Zumo. Setting the mode to 1 which is the driving mode.
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  time_since = millis();
  mode = 1;
}

void loop()
{ 
  //Starting the sensors on the reflectence sensor to read the values.
  position = sensors.readLine(sensor_values);
  sensors.read(sensor_values);

  if(mode == 1) {
    //If the mode is in drive, the start detecting the lines.
    //The line code will keep the Zumo in the corridor.
    detectLines();    
  }

  //Simple switch statement switching between different cases to satisfiy IF statements
  //On the Drive() function.
  switch(mode) {

    case 1:
      Drive();
      break;

    case 3:
      Drive();
      break;
      
    case 4:
      Drive();
      break;
      
    case 5:
      scanRoom();
      break;
  }
  
}


void startTurn() {
  if(mode == 1){
      Serial.println("Entering manual turning mode.");
      Serial.println("Press 'c' when complete.");
      Serial.println("\n Press 'e' to end");
      turn_count_l = 0;
      turn_count_r = 0;
      mode = 3;
  }
  else {
    if(path[path_length].dir == 'l') {
      AutoTurn(1600, 'r'); // turn the opposite way
    }
    else {
      AutoTurn(1600, 'l'); // turn the opposite way
    }
    path_length--;
  }  
}

void detectLines() {
  
  //Code used from the maze solver
  //If the second set of sensors are above the threshold of 400ms
  if(sensor_values[1] > QTR_THRESHOLD || sensor_values[4] > QTR_THRESHOLD) {
      //Motors need to be stopped as we have reached the end of a corridor.
      motors.setSpeeds(0,0);
      Serial.println("\nEnd of corridor.");
      //Call the turn function as we need to manually turn.
      startTurn();   
    }
    else if (sensor_values[0] > QTR_THRESHOLD && position < 2000)
    //If the furtherst sensor to the left is great than the threshold
    //And position is less than 2000 we have again hit the end of a corridor.
    {
      delay(50);
      sensors.read(sensor_values);
      if(sensor_values[1] > QTR_THRESHOLD) {
        motors.setSpeeds(0,0);
        Serial.println("\nEnd of corridor.");
        startTurn();   
      }
      else {
        //If the sensor furthest to the left detects a line we need to do an autocorrect to keep it inside the maze
        //We need to reverse the Zumo to go back off the line, then turn right and go forward.
        motors.setSpeeds(-SPEED, -SPEED);
        delay(DURATION);
        motors.setSpeeds(SPEED, -SPEED);
        delay(DURATION);
      }
    }
    else if (sensor_values[5] > QTR_THRESHOLD && position > 3000)
    //If the sensor furthest to the right detects a response greater than the threshold
    {
      delay(50);
      sensors.read(sensor_values);
      if(sensor_values[4] > QTR_THRESHOLD) {
        //And the second sensor in on the right has gone above the threshold
        motors.setSpeeds(0,0);
        //We stop the motors on the Zumo
        Serial.println("\nEnd of corridor.");
        //And know we have hit the end of a corridor.
        startTurn();
        //Call the startTurn function to manually turn the Zumo.
      }
      else {
        //If the sensor furthest to the right detects a line
        //We want to reverse and turn the Zumo left.
        motors.setSpeeds(-SPEED, -SPEED);
        delay(DURATION);
        motors.setSpeeds(-SPEED, SPEED);
        delay(DURATION);
        //motors.setSpeeds(SPEED,  SPEED);
      }
    }
}

void roomTurn() 
{
  turn_count_l = 0;
  turn_count_r = 0;
}

//In my program this does the bulk of the work. In here I can go forward, backwards and turn either side.
void Drive() {
  String dir;
  
  if (Serial.available() > 0) 
  {
    time_since = millis();
    c = Serial.read();  //Serial read
    switch(c) {
      case 'W':
      case 'w':
        if(p == false) {
          s = millis();
          p = true;
        }
        motors.setSpeeds(SPEED, SPEED);
        delay(2);
        break;

      case 'a':
      case 'A':
        if(mode == 3 || mode == 4) {
          turn_count_l++;
        }
        motors.setSpeeds(-SPEED, SPEED);
        break;

      case 's':
      case 'S':
        motors.setSpeeds(-SPEED, -SPEED);
        break;

      case 'd':
      case 'D':
      case 78:
        if(mode == 3 || mode == 4) {
          turn_count_r++;
        }
        motors.setSpeeds(SPEED, -SPEED);
        break;

      case 'r':
        Serial.println("Room Mode.");
        Serial.println("\nTurn robot and wait");
        delay(1000);
        roomTurn();
        mode = 4;
        break;

      case 'c':
        if(mode == 3) {
          if(turn_count_r > turn_count_l) {
            Serial.print("\nRight");
            path[path_length].dir = 'r';
          }  
          else {
            Serial.print("\nLeft");
            path[path_length].dir = 'l';
          }
          path[path_length].type = 't';
          Serial.print("Complete. Entering Drive mode.");
          path_length++;
          mode = 1;
        }
        else {
          Serial.println("Drive Mode."); 
          mode = 1;
        }
        break;

     default:
        motors.setSpeeds(0, 0);
        break;
    }    
  }
  t = millis();
  if(t - time_since > 200) {
    motors.setSpeeds(0,0);
    if(p==true) {
      
      time_move += t-s;
      p = false;
    }
  }
  if(mode == 4) {
    if(t - time_since > 3000) {
      if(turn_count_r > turn_count_l) {
        Serial.print("\nRIGHT");
        path[path_length].dir = 'r';
      }     
      else {
        Serial.print("\nLEFT");
        path[path_length].dir = 'l';
      }

      path[path_length].type = 'r';
      path[path_length].room_no = room_count;
      Serial.print(" room");
      Serial.print(" - no.");
      Serial.print(room_count);

      room_count++;
      
      delay(1000); 
      mode = 5;
    }
  }
}

void scanRoom() {
  bool found = false;
  Serial.print("\nRoom scanning started.");
  //Automatically start the motors to go forward.
  motors.setSpeeds(100, 100);
  delay(600);
  motors.setSpeeds(0,0);

  delay(1000);

  //Start the Zumo turning right.
  motors.setSpeeds(SPEED, -SPEED);
  delay(800);
  motors.setSpeeds(0,0);
  delay(200);

  //Start the Zumo turning left and starts scanning.
  motors.setSpeeds(-SPEED, SPEED); 
  //delay(1750);
  for(int i=0; i<35; i++) {
    if(sonar.ping_cm() <= 30 && sonar.ping_cm() > 0)
    //If an item has been found in the CM radius bool is set to true.
      found = true;
    delay(50);
  }

  motors.setSpeeds(0,0);
  delay(200);

  //Start Zumo turning right and scanning initiated.
  motors.setSpeeds(SPEED, -SPEED); 
  //delay(2000);
  for(int i=0; i<40; i++) {
    if(sonar.ping_cm() <= 30 && sonar.ping_cm() > 0)
      found = true;
    delay(50);
  }

  Serial.print(".");
  motors.setSpeeds(0,0);
  delay(200);
  Serial.print(".");
  
  motors.setSpeeds(-SPEED, SPEED); // turn left
  delay(800);
  motors.setSpeeds(0,0);
  Serial.println("Complete");
  
  if(found) {
    if(mode != 6)
      Serial.println("Object has been found");
    else
      Serial.println("OBJECT STILL IN ROOM");
  }
  else {
    if(mode != 6)
      Serial.println("No object found.");
    else
      Serial.println("ROOM CLEARED");
  }

  if(mode != 6){
    path[path_length].item = found;
    path_length++;
    
    Serial.println("Please move Zumo back into corridor.");
    Serial.println("Press 'c' when complete.");
    mode = 1;
  }  
}


void AutoTurn(long ms, char dir) {
  //Automated turning function.
  //If the direction is left then we want to turn left.
  if(dir== 'l')
    motors.setSpeeds(-SPEED, SPEED);
  else
  //If the direction is right we want to turn right.
    motors.setSpeeds(SPEED, -SPEED);

  delay(ms);
  motors.setSpeeds(0,0);
}


