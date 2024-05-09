// MREN 303 Pico W Wifi Gamepad Read and Switch Modes
// Written For MREN 303 

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>
#include <Wire.h>

#ifndef STASSID
#define STASSID "MREN303_wifi"
#define STAPSK "MREN@303p1"
#endif


unsigned int localPort = 8888;  // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1];  // buffer to hold incoming packet,
char ReplyBuffer[22] = "acknowledged\r\n";        // a string to send back
// char Arrays to match commands from gamepad 
char stringA[17] =    "A Button Press";
char stringB[17] =    "B Button Press";
char stringY[17] =    "Y Button Press";
char stringX[17] =    "X Button Press";
char stringLB[17] =   "LB Button Press";
char stringRB[17] =   "RB Button Press";
char stringLS[17] =   "L Stick Press";
char stringRS[17] =   "R Stick Press";
char stringBack[17]=  "Back Press";
char stringStart[17]= "Start Press";
char stringUp[17] =   "Up Dpad Press";
char stringDown[17] = "Down Dpad Press";
char stringLeft[17] = "Left Dpad Press";
char stringRight[17]= "Right Dpad Press";
char stringLX[17] =   "L Stick X Value ";
char stringLY[17] =   "L Stick Y Value ";
char stringRX[17] =   "R Stick X Value ";
char stringRY[17] =   "R Stick Y Value ";
char stringLT[17] =   "L Trigger Value ";
char stringRT[17] =   "R Trigger Value ";

int valueLx = 0;
int valueLy = 0;
int valueRx = 0;
int valueRy = 0;
int valueLt = 0;
int valueRt = 0;
bool ledOn = false;

//DC motor, servo, and sensor initialization
Servo servo1;
Servo servo2;
Servo servo3;

//Initialize Pins for Servos
int servo1Pin = 0;
int servo2Pin = 1;
int servo3Pin = 2;
int pos = 0;

// //Initializing pins for DC motors
const int pwmA = 28;
const int in1A = 6;
const int in2A = 27;
const int standby = 22;
const int ENCA1 = 21;
const int ENCA2 = 20;
const int ENCB1 = 13;
const int ENCB2 = 12;
const int pwmB = 17;
const int in1B = 19;
const int in2B = 18;
const int LED = 25;


//Initializing vaiables for speed control
double currPosA = 0;
double currPosB = 0;
double prevPosA;
double prevPosB;
int count = 0;
int eA = 0;
int eB = 0;
int direcA = 0;
int direcB = 0;
int stage = 0;
float uA = 0;
float uB = 0;
int stopCount = 0;

double SetpointA, SetpointB;
//define array's for left motor and right motor positions as well as accepted error
//sequence as follows: Forwards, rotate clockwise, backwards, rotate counterclockwise, backwards, set to stage 2, forwards moving clockwise, backwards into button, set to stage 4, forwards counter clockwise, backwards clockwise through doors.
float arrA[] = {760, 356, -555, 0, -555, 1, 530, -250, 1, 470, 230, -630, 1};
float arrB[] = {610, -350, -540, 350, -575, 1, 120, -720, 1, 600, -100, -600, 1};
float err[] = {65, 65, 65, 65, 65, 1, 65, 65, 1, 65, 35, 65, 1};

//defines pins numbers for ultrasound 2
const int trigPin2 = 10;
const int echoPin2 = 15;

//defines variables for ultrasound
long duration2;
int distance2;

//define variables used for controller interaction 
bool standbyButton = false;
long currT = 0;
long T = 0;
long firstT = 0;
bool firstCond = false;
int y = 0;
int x = 0;
int rightBump = 0;
int leftBump = 0;
bool LTBool = false;
bool RTBool = false;
int servoVal1 = 45;
int servoVal2 = 45;
int servoVal3 = 45;


WiFiUDP Udp;

void setup() { //This handles network function and runs on core 1
  Serial.begin(115200);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP server on port %d\n", localPort);
  Udp.begin(localPort);
}
void setup1(){ //This handles all inputs and outputs and runs on core 2
  
  //Initialize servo operating range
  servo1.attach(servo1Pin, 500, 2500);
  servo2.attach(servo2Pin, 500, 2500);
  servo3.attach(servo3Pin, 500, 2500);

  //Initialize distance sensor pins
  pinMode(trigPin2, OUTPUT); 
  pinMode(echoPin2, INPUT); 

  //flash LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  //Set encoder inputs
  pinMode(ENCA1, INPUT);
  pinMode(ENCA2, INPUT);
  pinMode(ENCB1, INPUT);
  pinMode(ENCB2, INPUT);
  //Make sure an interrupt is called avery time encoder 1 rises
  attachInterrupt(digitalPinToInterrupt(ENCA1), readEncoderA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCB1), readEncoderB, RISING);

  // Initialize motor driver pins as outputs
  pinMode(pwmA, OUTPUT);
  pinMode(in1A, OUTPUT);
  pinMode(in2A, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(in1B, OUTPUT);
  pinMode(in2B, OUTPUT);
  pinMode(standby, OUTPUT);
  digitalWrite(standby, HIGH);
}

void loop() { //This loop executes on Core 1 of the Pico, handles networking and recieves commands
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d)\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort(), Udp.destinationIP().toString().c_str(), Udp.localPort());

    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;

    char comparisonString[17] = "";
    char valueString[5] = "";

    for (int i=0; i<16 ; i++){
      comparisonString[i]=packetBuffer[i];
    }
    comparisonString[16] = 0;
    
    Serial.println("Comp String:");
    Serial.println(comparisonString);

    //if the a button is pressed turn state of ledOn to true (this turns on autonomous mode)
    if (strcmp(comparisonString,stringA) == 0){
      Serial.println("Autonomous Mode:");
      ledOn = true;
      firstT = millis();           
    }
    //if the b button is pressed turn state of ledOn to false (this turns on manual mode)
    if (strcmp(comparisonString,stringB) == 0){
      Serial.println("Manual Mode");
      ledOn = false;
    }
    //if the x button is pressed switch state of x (this controls backwards motion of servo one)
    if (strcmp(comparisonString,stringX) == 0){
      if (x == 0){
        x = 1;
      }
      else{
        x = 0;
      }
    }
    //if the y button is pressed switch state of y (this controls forwards motion of servo one)
    if (strcmp(comparisonString,stringY) == 0){
      if (y == 0){
        y = 1;
      }
      else{
        y = 0;
      }
    }
    //if the left bumper is pressed switch state of leftBump (this controls forwards motion of servo two)
    if (strcmp(comparisonString,stringLB) == 0){
      Serial.println("Manual Mode");
      if (leftBump == 0){
        leftBump = 1;
      }
      else{
        leftBump = 0;
      }
    }
    //if the right bumper is pressed switch state of rightBump (this controls backwards motion of servo two)
    if (strcmp(comparisonString,stringRB) == 0){
      Serial.println("Manual Mode");
      if (rightBump == 0){
        rightBump = 1;
      }
      else{
        rightBump = 0;
      }
    }
    //if start button is pressed go into standby mode (motor driver stops drawing power)
    if (strcmp(comparisonString,stringStart) == 0){
      if (standbyButton){
       standbyButton  = false;
       digitalWrite(standby, HIGH);
      }
      else{
        standbyButton = true;
        digitalWrite(standby, LOW);
      }
    }
    //check if left joystick is moved in x direction
    if (strcmp(comparisonString,stringLX) == 0){
      for(int i=16; i<=packetSize ; i++){
        valueString[(i-16)] = packetBuffer[i];}
      valueString[5] = 0;
      //assign value from 0 to 255 to valueLx
      valueLx = atoi(valueString);
    }
    //check if left joystick is moved in y axis
    if (strcmp(comparisonString,stringLY) == 0){
      for(int i=16; i<=packetSize ; i++){
        valueString[(i-16)] = packetBuffer[i];}
      valueString[5] = 0;
      //assign value from 0 to 255 to valueLy
      valueLy = atoi(valueString);
    }
    //if the left stick button is pressed switch state of LTBool (this controls forwards motion of servo three)
    if (strcmp(comparisonString, stringLS) == 0) {
      Serial.println("Reset");
      if (LTBool){
       LTBool  = false;
      }
      else{
        LTBool = true;
      }
    }
    //if the right stick button is pressed switch state of RTBool (this controls backwards motion of servo three)
    if (strcmp(comparisonString, stringRS) == 0) {
      Serial.println("Reset");
      if (RTBool){
        RTBool = false;
      }
      else{
        RTBool = true;
      }
    }
    //check if right joystick is moved in the x axis
    if (strcmp(comparisonString,stringRX) == 0){
      for(int i=16; i<=packetSize ; i++){
        valueString[(i-16)] = packetBuffer[i];}
      valueString[5] = 0;
      //assign value from 0 to 255 to valueRx
      valueRx = atoi(valueString);
    }
    //check if right joystick is moved in the y axis
    if (strcmp(comparisonString,stringRY) == 0){
      for(int i=16; i<=packetSize ; i++){
        valueString[(i-16)] = packetBuffer[i];}
      valueString[5] = 0;
      //assign value from 0 to 255 to valueLx
      valueRy = atoi(valueString);
    }
    //check if right trigger is pressed
    if (strcmp(comparisonString,stringRT) == 0){
      for(int i=16; i<=packetSize ; i++){
        valueString[(i-16)] = packetBuffer[i];}
      valueString[5] = 0;
      //assign value from 0 to 255 to valueRt
      valueRt = atoi(valueString);
    }
    //check if left trigger is pressed
    if (strcmp(comparisonString,stringLT) == 0){
      for(int i=16; i<=packetSize ; i++){
        valueString[(i-16)] = packetBuffer[i];}
      valueString[5] = 0;
      //assign value from 0 to 255 to valueLt
      valueLt = atoi(valueString);
    }
    
    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
}

void loop1(){ //This Runs on Core 2 and performs all inputs and outputs
  //Autonomous mode
  if(ledOn){
    digitalWrite(LED_BUILTIN,1);
    //Get current time and subtract it by the time when autonomous mode first gets triggered to determine how long you are in autonomous mode
    currT = millis();
    T = currT - firstT;
    //If the time in autonomous mode becomes greater than 30 seconds exit autonomous mode
    if (T >= 30000){ ledOn = false; }
    currPosA = 0;
    currPosB = 0;

    //first stage of autonomous mode
    if (stage == 0){
      //while loop reading encoder values obtained from readEncoderA and readEncoderB
      while(1){
        //Get the desired position for each motor from array of values
        SetpointA = arrA[count];
        SetpointB = arrB[count];
        //Calculate the error between the desired position and current position (in terms of encoder ticks)
        eA = 0.30*(SetpointA - currPosA);
        eB = 0.30*(SetpointB - currPosB);

        //find direction that each motor should go depending on if error is negative or positive
        if (eA > 0){
          direcA = 1;
        }
        else {
          direcA = -1;
        }
        if (eB > 0){
          direcB = 1;
        }
        else {
          direcB = -1;
        }

        //make sure robot dosin't go too fast
        if (fabs(eA) > 200){ eA = 200;}
        if (fabs(eB) > 200){ eA = 200;}
        //Call setMotor function to assign desired speed to motor
        setMotor(direcA, fabs(eA) + 55, pwmA, in1A, in2A);
        setMotor(direcB, fabs(eB) + 55, pwmB, in1B, in2B);

        //if the error is smaller than the desired error robot is in range and move on to the next waypoint by incrementing the count
        if (err[count] > fabs(eB) && err[count] > fabs(eA)) {
          //make sure encoder ticks (which indicate our position) are set back to zero. 
          currPosA = 0;
          currPosB = 0;
          //stop robot for half a second
          setMotor(direcA, 0, pwmA, in1A, in2A);
          setMotor(direcB, 0, pwmB, in1B, in2B);
          //increment the count to access next waypoint found in the array's
          count++;
          delay(600);
        }
        //check to see if the next waypoint is 1 or if 30 seconds has passed, if so, move on to next stage and break out of loop
        currT = millis();
        if ((SetpointA == 1 && SetpointB == 1) || (currT - firstT >= 30000)) {
          setMotor(direcA, 0, pwmA, in1A, in2A);
          setMotor(-direcA, 0, pwmB, in1B, in2B);
          count = count + 1;
          stage = 1;
          currPosA = 0;
          currPosB = 0;
          break;
        }
      }
    }
    //second stage of autonomous mode
    else if (stage == 1){
      //drive robot backwards at a speed of 150 for 2000
      //this drives it into the wall so that it can be completely hozirontal eliminating any potential discepancies from the first stage 
      setMotor(-1, 150, pwmA, in1A, in2A);
      setMotor(-1, 150, pwmB, in1B, in2B);
      delay(500);
      stage = 2;
      currPosA = 0;
      currPosB = 0;
    }

    //third stage of autonomous mode (same as first stage)
    else if (stage == 2){
      while(1){
        SetpointA = arrA[count];
        SetpointB = arrB[count];
        eA = 0.30*(SetpointA - currPosA);
        eB = 0.30*(SetpointB - currPosB);

        if (eA > 0){
          direcA = 1;
        }
        else {
          direcA = -1;
        }
        if (eB > 0){
          direcB = 1;
        }
        else {
          direcB = -1;
        }

        if (fabs(eA) > 200){ eA = 200;}
        if (fabs(eB) > 200){ eA = 200;}
        setMotor(direcA, fabs(eA) + 55, pwmA, in1A, in2A);
        setMotor(direcB, fabs(eB) + 55, pwmB, in1B, in2B);

        if (err[count] > fabs(eB) && err[count] > fabs(eA)) {
          currPosA = 0;
          currPosB = 0;
          setMotor(direcA, 0, pwmA, in1A, in2A);
          setMotor(direcB, 0, pwmB, in1B, in2B);
          count++;
          delay(600);
        }

        currT = millis();
        if ((SetpointA == 1 && SetpointB == 1) || (currT - firstT >= 30000)) {
          Serial.println("timeOut");
          setMotor(direcA, 0, pwmA, in1A, in2A);
          setMotor(-direcA, 0, pwmB, in1B, in2B);
          count = count + 1;
          stage = 3;
          currPosA = 0;
          currPosB = 0;
          break;
        }
      } 
    }
    
    //fourth stage of autonomous mode
    //like second stage run motors for 2 seconds forwards to get robot to align with the wall (at this stage the robot would be ramming into the button)
    else if (stage == 3){
      setMotor(-1, 140, pwmA, in1A, in2A);
      setMotor(-1, 160, pwmB, in1B, in2B);
      delay(700);
      stage = 4;
      currPosA = 0;
      currPosB = 0;
    } 

    //fifth stage (same as first and third stages except break out of autonomous mode once done)
    else if (stage == 4){
      while(1){
        SetpointA = arrA[count];
        SetpointB = arrB[count];
        eA = 0.30*(SetpointA - currPosA);
        eB = 0.30*(SetpointB - currPosB);

        if (eA > 0){
          direcA = 1;
        }
        else {
          direcA = -1;
        }
        if (eB > 0){
          direcB = 1;
        }
        else {
          direcB = -1;
        }

        if (fabs(eA) > 200){ eA = 200;}
        if (fabs(eB) > 200){ eA = 200;}
        setMotor(direcA, fabs(eA) + 55, pwmA, in1A, in2A);
        setMotor(direcB, fabs(eB) + 55, pwmB, in1B, in2B);

        if (err[count] > fabs(eB) && err[count] > fabs(eA)) {
          currPosA = 0;
          currPosB = 0;
          setMotor(direcA, 0, pwmA, in1A, in2A);
          setMotor(direcB, 0, pwmB, in1B, in2B);
          count++;
          stopCount = 0;
          delay(600);
        }

        currT = millis();
        if ((SetpointA == 1 && SetpointB == 1) || (currT - firstT >= 30000)) {
          Serial.println("timeOut");
          setMotor(direcA, 0, pwmA, in1A, in2A);
          setMotor(-direcA, 0, pwmB, in1B, in2B);
          count = 0;
          stage = 0;
          currPosA = 0;
          currPosB = 0;
          ledOn = false;
          break;
      }
    } 
  } 
}
  //Manual mode (if lenOn is false)
  else {
    digitalWrite(LED_BUILTIN,0);
    //receive values in a range from 0 to 255 from the left joisticks y axis
    int speedLeft = valueLy;
    int speedRight = valueLy;
    //receive values in a range from 0 to 255 from the right joistick x axis
    int steer = valueRx;
    float sens = 1.5;

    //Subtrack or add (depending on sign) steer value to the speed of each motor
    //Use sens value to tune the control making it smoother
    speedLeft = (speedLeft/1.2) + (steer/sens);
    speedRight = (speedRight/1.2) - (steer/sens);

    //Make sure motors A(left) and B(right) power doesn't go above 200
    float mpwrA = fabs(speedLeft);
    if(mpwrA>200){
      mpwrA = 200;
    }
    float mpwrB = fabs(speedRight);
    if(mpwrB>200){
      mpwrB = 200;
    }

    //Set motors A(left) and B(right) direction depending on if control signal value is negative or positive
    int mdirecA = 1;
    if(speedLeft<0){
      mdirecA = -1;
    }
    int mdirecB = 1;
    if(speedRight<0){
      mdirecB = -1;
    }

    //If x is equal to one servo2 gets decremented
    if(x == 1){
      servoVal1 = servoVal1 - 1;
      if (servoVal1 < 0){
        servoVal1 = 0;
      }
    }

    //If y is equal to one servo1 gets incremented
    if(y == 1){
      servoVal1 = servoVal1 + 1;
      if (servoVal1 > 150){
        servoVal1 = 150;  
      }
    } 

    //If leftBump is equal to one servo2 gets incremented
    if(leftBump == 1){
      servoVal2 = servoVal2 + 1;
      if (servoVal2 > 80) {
        servoVal2 = 80;
      } 
    }

    //If rightBump is equal to one servo2 gets decremented
    if(rightBump == 1){
      servoVal2 = servoVal2 - 1;
      if (servoVal2 < 30) {
        servoVal2 = 30;
      }  
    }

    //If RTBool is true and servo3 gets incremented
    if(RTBool){ 
      servoVal3 = servoVal3 + 2;
      if (servoVal3 > 90) {
        servoVal3 = 90;
      }  
    }

    //If LTBool is true servo3 gets decremented
    if(LTBool){
      servoVal3 = servoVal3 - 1;
      if (servoVal3 < 10) {
        servoVal3 = 10;
      }  
    }

    //Write the inputed controller values to each servo.
    servo1.write(servoVal1);
    servo2.write(servoVal2);
    servo3.write(servoVal3);

    //Call setMotor function to assign desired speed to motor
    setMotor(mdirecA, mpwrA, pwmA, in1A, in2A);
    setMotor(mdirecB, mpwrB, pwmB, in1B, in2B);
  }
  delay(100);
}



//Using inputted direction and power values set the motor directions and speeds
void setMotor(int dir, int pwmVal, int pwm, int in1, int in2){
  analogWrite(pwm, pwmVal);
  if(dir == 1){
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
  else if(dir == -1){
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
  else{
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
}

//Find out if encoder 2 is either trailing encoder 1 (rising slightly after it) or leading encoder 1 (rising slightly before it).
//With this information we know the direction the motor is turning and we can count the amount of encoder ticks.
//Left wheel encoder reading.
void readEncoderA(){ 
  if(digitalRead(ENCA2) > 0){
    currPosA++;
  }
  else{
    currPosA--;
  }
}

//Right wheel encoder reading.
void readEncoderB(){
  if(digitalRead(ENCB2) > 0){
    currPosB--;
  }
  else{
    currPosB++;
  }
}


