#include <math.h>
#include <MsTimer2.h>
#include <SPI.h>
#include <CAN.h>

/*----------------
 * How to control motor
 * 
 * 1. 1A(HIGH) && 2A(LOW) >> reverse clock direction
 * 2. 1A(LOW) && 2A(HIGH) >> clock direction
 * 3. 3B(HIGH) && 4B(LOW) >> reverse clock direction
 * 4. 3B(LOW) && 4B(HIGH) >> clock direction
 * 
 */

//  Assigning pin numbers
//------------------------------------------------

#define encoderL   18
#define encoderL_g 19
#define encoderR   21
#define encoderR_g 20

#define EA  6
#define A1  12
#define A2  11

#define B3  10
#define B4   9
#define EB   8

#define trig 37
#define echo 33

//   generate variables
//------------------------------------------------
boolean left_steering, right_steering;
int vel_L = 100, vel_R = 100;

const int ppr = 1800;
volatile int pulseCountL = 0, pulseCountR = 0;
volatile int rpmL, rpmR;

volatile const float Kp = 1.1;
volatile const float Kd = 1.1;

volatile int error, speed_gap;
volatile float prev_error = 0;
volatile double Pcontrol, Dcontrol, PIDcontrol;

float reflect_duration, obstacle_distance, velocity, ttc, rpm;
short can_lock = 0;
int target_gap;
char rxBuffer;

//   interrupt function definitions
//------------------------------------------------
void speed_limit()
{
    if(vel_L > 250) vel_L = 250;
    else if(vel_L < 50) vel_L = 50;
        
    if(vel_R > 250) vel_R = 250;
    else if(vel_R < 50) vel_R = 50;
}


void speedCalibration()
{
  rpmL = int(pulseCountL / 0.5 / ppr) * 60;
  rpmR = int(pulseCountR / 0.5 / ppr) * 60;

  if(left_steering)
  {
    speed_gap = rpmR - rpmL;
    error = speed_gap - target_gap;
    Pcontrol = Kp * error;
    Dcontrol = Kd * (error - prev_error);
    PIDcontrol = Pcontrol + Dcontrol;
    prev_error = error;
    if(speed_gap > target_gap)      vel_R += PIDcontrol;
    else if(speed_gap < target_gap) vel_R -= PIDcontrol;
    speed_limit();
    speedSetup(vel_L, vel_R);
  }
  else if(right_steering)
  {
    speed_gap = rpmL - rpmR;
    error = speed_gap - target_gap;
    Pcontrol = Kp * error;
    Dcontrol = Kd * (error - prev_error);
    PIDcontrol = Pcontrol + Dcontrol;
    prev_error = error;
    if(speed_gap > target_gap)      vel_L += PIDcontrol;
    else if(speed_gap < target_gap) vel_L -= PIDcontrol;
    speed_limit();
    speedSetup(vel_L, vel_R);
  }
  else
  {
    if(rpmL != rpmR)
    {
      if(rpmL < rpmR) vel_L += 1;
      else            vel_R += 1;
      speed_limit();
      speedSetup(vel_L, vel_R);
    }
  }

  Serial.print("Left rpm: ");
  Serial.println(rpmL);
  Serial.print("Right rpm: ");
  Serial.println(rpmR);
  Serial.print("left speed : ");
  Serial.println(vel_L);
  Serial.print("right speed : ");
  Serial.println(vel_R);
    
  pulseCountL = 0;
  pulseCountR = 0;
}

void pulseCounterL() { pulseCountL++; }
void pulseCounterR() { pulseCountR++; }


//   function definitions
//------------------------------------------------
void speedSetup(int left, int right)
{
  analogWrite(EA, left);
  analogWrite(EB, right);
}

//direction set to move forward
void moveFront()
{
  digitalWrite(A2, LOW);
  digitalWrite(A1, HIGH);
  digitalWrite(B4, LOW);
  digitalWrite(B3, HIGH);
}


//  1. Setup all pins as output
//  2. Push and pull topics
//----------------------------------------------
void setup()
{
  pinMode(encoderL, INPUT);
  pinMode(EA, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A1, OUTPUT);

  pinMode(encoderR, INPUT);
  pinMode(EB, OUTPUT);
  pinMode(B3, OUTPUT);
  pinMode(B4, OUTPUT);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  moveFront();
  
  Serial.begin(9600);
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }

  attachInterrupt(digitalPinToInterrupt(encoderL), pulseCounterL, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderR), pulseCounterR, RISING);
   
  MsTimer2::set(200, speedCalibration);
  MsTimer2::start();

  speedSetup(0, 0);//initial speed >> 0
}


//   Publish received data from Jetson TX2
//---------------------------------------------
void loop()
{    
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  reflect_duration = pulseIn(echo, HIGH);
  obstacle_distance = ((float)(340 * reflect_duration) / 10000) / 2;
  rpm = (rpmL + rpmR) / 2;
  velocity = (rpm * 6.6 * 3.141592) / 60;
  ttc = (obstacle_distance / velocity) - 1.0;// 1 is system delay
  if(velocity <= 0) ttc = 0;
  if(ttc < 0) ttc = 0;
  Serial.print("distance : ");
  Serial.println(obstacle_distance);
  Serial.print("velocity : ");
  Serial.println(velocity);
  Serial.print("ttc : ");
  Serial.println(ttc);

  if(ttc <= 2)
  {
    can_lock = 1;
    speedSetup(0, 0);
  }
  else if(ttc <= 3)
  {
    can_lock = 0;
    speedSetup(vel_L - 50, vel_R - 50);
  }
  else if(ttc <= 4)
  {
    can_lock = 0;
    speedSetup(vel_L - 30, vel_R - 30);
  }
  else
  {
    can_lock = 0;
    speedSetup(vel_L, vel_R);
  }

  if(!can_lock)
  {
    if(CAN.parsePacket())
    {
      if(CAN.available())
      {
        rxBuffer = CAN.read();
        target_gap = (signed int)rxBuffer;
        Serial.print("auto command : ");
        Serial.println(target_gap);
        if(target_gap > 0)
        {
            left_steering = true;
            right_steering = false;
        }
        else if(target_gap < 0)
        {
            left_steering = false;
            right_steering = true;
        }
        else
        {
            left_steering = false;
            right_steering = false;
            vel_L = 100;
            vel_R = 100;
        }
      }
    }
  }
}
