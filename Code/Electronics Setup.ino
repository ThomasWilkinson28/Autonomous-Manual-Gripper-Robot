#include <Servo.h>
#include <servo.pio.h>
#include <servo.h>

Servo servo1;
Servo servo2;
Servo servo3;

//Initialize Pins for Servos
int servo1Pin = 0;
int servo2Pin = 1;
int servo3Pin = 2;

int pos = 0;

//Initializing pins for DC motors
const int pwmA = 28;
const int in1A = 26;
const int in2A = 27;
const int standby = 22;
const int ENCA1 = 21;
const int ENCA2 = 20;
const int pwmB = 17;
const int in1B = 19;
const int in2B = 18;
const int ENCB1 = 13;
const int ENCB2 = 12;
const int LED = 25;

// defines pins numbers for ultrasound 1
const int trigPin1 = 11;
const int echoPin1 = 14;
// defines pins numbers for ultrasound 2
const int trigPin2 = 10;
const int echoPin2 = 15;
// defines pins numbers for ultrasound 3
const int trigPin3 = 9;
const int echoPin3 = 16;

//Initializing vaiables for speed control
int currPosA = 0;
long prevTA = 0;
float eprevA = 0;
float eintegralA = 0;
int currPosB = 0;
long prevTB = 0;
float eprevB = 0;
float eintegralB = 0;

// defines variables for ultrasounds
long duration1;
int distance1;
long duration2;
int distance2;
long duration3;
int distance3;

void setup() {
  Serial.begin(9600);

  servo1.attach(servo1Pin, 500, 2500);
  servo2.attach(servo2Pin, 500, 2500);
  servo3.attach(servo3Pin, 500, 2500);

  pinMode(trigPin1, OUTPUT); 
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT); 
  pinMode(echoPin2, INPUT); 
  pinMode(trigPin3, OUTPUT); 
  pinMode(echoPin3, INPUT);
  
  //Get onboard LED to flash indicating program has started
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

// Set all the motor control pins to outputs
  pinMode(pwmA, OUTPUT);
  pinMode(in1A, OUTPUT);
  pinMode(in2A, OUTPUT);
  pinMode(pwmB, OUTPUT);
  pinMode(in1B, OUTPUT);
  pinMode(in2B, OUTPUT);
  pinMode(standby, OUTPUT);
//Set standby to be always HIGH
  digitalWrite(standby, HIGH);
}

void loop() {
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration1 = pulseIn(echoPin1, HIGH);

  // Calculating the distance
  distance1 = duration1 * 0.034 / 2;

  // Prints the distance on the Serial Monitor
  Serial.print("Distance 1 (cm): ");
  Serial.println(distance1);

  // For second sensor
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  
  duration2 = pulseIn(echoPin2, HIGH);

  distance2 = duration2 * 0.034 / 2;

  Serial.print("Distance 2 (cm): ");
  Serial.println(distance2);

  //For third sensor
  digitalWrite(trigPin3, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin3, LOW);
  
  duration3 = pulseIn(echoPin3, HIGH);

  distance3 = duration3 * 0.034 / 2;

  Serial.print("Distance 3 (cm): ");
  Serial.println(distance3);

  //Set target position for motor to follow
  int targetPosA = 250*sin(prevTA/1e6);
  int targetPosB = 250*sin(prevTB/1e6);

  //PID constants
  float kpA = 1;
  float kdA = 0.16;
  float kiA = 0.4;
  float kpB = 1;
  float kdB = 0.16;
  float kiB = 0.4;

  //Getting delta t for integral and derivative control
  long currTA = micros();
  long currTB = micros();

  float deltaTA = ((float)(currTA - prevTA))/1.0e6;
  prevTA = currTA;
  float deltaTB = ((float)(currTB - prevTB))/1.0e6;
  prevTB = currTB;

  //Calculate error between target position and current position
  int eA = targetPosA - currPosA;
  int eB = targetPosB - currPosB;
   
  //Calculate error derivative using delta t found above
  float ederiveA = (eA - eprevA)/(deltaTA);
  float ederiveB = (eB - eprevB)/(deltaTB);

  //Calculate error integral using delta t found above
  eintegralA = eintegralA + eA*deltaTA;
  eintegralB = eintegralB + eB*deltaTB;

  //Control signal equation
  float uA = kpA*eA + kdA*ederiveA + kiA*eintegralA;
  float uB = kpB*eB + kdB*ederiveB + kiB*eintegralB;

  //Make sure motor power doesn't go above 255
  float pwrA = fabs(uA);
  if(pwrA>255){
    pwrA = 255;
  }

  float pwrB = fabs(uB);
  if(pwrB>255){
    pwrB = 255;
  }

  //Set motor direction depending on if control signal value is negative or positive
  int direcA = 1;
  if(uA<0){
    direcA = -1;
  }

  int direcB = 1;
  if(uB<0){
    direcB = -1;
  }

  //Call setMotor function to assign desired speed to motor
  setMotor(direcA, pwrA, pwmA, in1A, in2A);
  setMotor(direcB, pwrB, pwmB, in1B, in2B);

  //Store prev error for next time arround
  eprevA = eA;
  eprevB = eB;
  delay(100);

  pos = abs(targetPosA/4);
  servo1.write(pos);             
  servo2.write(pos);
  servo3.write(pos);

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
//With this information we know the direction the motor is turning and we can return that it is either moving forward or backward.
void readEncoderA(){
  int b = digitalRead(ENCA2);
  if(b>0){
    currPosA++;
  }
  else{
    currPosA--;
  }
}

void readEncoderB(){
  int b = digitalRead(ENCB2);
  if(b>0){
    currPosB++;
  }
  else{
    currPosB--;
  }
}
