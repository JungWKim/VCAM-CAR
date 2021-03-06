#include <stdio.h>
#include <MsTimer2.h>
#include <mcp_can.h>
#include <SPI.h>

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

#define encoderL    2
#define encoderL_g  3
#define encoderR   21
#define encoderR_g 20

#define EA  13
#define A1  12
#define A2  11

#define B3  10
#define B4   9
#define EB   8

#define trig 37
#define echo 33

MCP_CAN CAN0(10);

//   generate variables
//------------------------------------------------
int buf;/*buffer to store previous state*/ 
int vel_L = 100, vel_R = 100;
unsigned char can_tx_buffer[1];
unsigned char can_rx_buffer;
unsigned char len = 0;
long unsigned int rxId;
volatile int leftTargetSpeed, rightTargetSpeed;

const int ppr = 50;
volatile int pulseCountL = 0, pulseCountR = 0;
volatile int rpmL, rpmR, rpm;

const float Kp = 1.;
const float Ki = 1.;
const float Kd = 1.;

volatile int errorL, errorR;
volatile float prev_errorL = 0, prev_errorR = 0;
volatile float integral_errorL = 0, integral_errorR = 0;

volatile double PcontrolL, IcontrolL, DcontrolL, PIDcontrolL;
volatile double PcontrolR, IcontrolR, DcontrolR, PIDcontrolR;

float duration, distance;
unsigned int ttc;



//   interrupt function definitions
//------------------------------------------------

void calculateRpm()
{
  rpmL = int(pulseCountL / 0.5 / ppr) * 60;
  rpmR = int(pulseCountR / 0.5 / ppr) * 60;
  rpm = (int)((rpmL + rpmR) / 2);
  Serial.print("Left rpm: ");
  Serial.println(rpmL);
  Serial.print("Right rpm: ");
  Serial.println(rpmR);
  pulseCountL = 0;
  pulseCountR = 0;
}


int convertSpeed2Rpm(int input)
{
  if(input <= 50)       return 60;
  else if(input <= 60)  return 240;
  else if(input <= 70)  return 360;
  else if(input <= 80)  return 420;
  else if(input <= 90)  return 540;
  else if(input <= 100) return 600;
  else if(input <= 110) return 660;
  else if(input <= 120) return 720;
  else if(input <= 130) return 780;
  else if(input <= 140) return 780;
  else if(input <= 150) return 840;
  else if(input <= 160) return 840;
  else if(input <= 170) return 900;
  else if(input <= 180) return 900;
  else if(input <= 190) return 900;
  else if(input <= 200) return 960;
  else if(input <= 210) return 960;
  else if(input <= 220) return 960;
  else if(input <= 230) return 1020;
  else if(input <= 240) return 1020;
  else                  return 1080;
}


void PID_L()
{
  int leftTargetRpm = convertSpeed2Rpm(leftTargetSpeed);
  errorL = leftTargetRpm - rpmL;
  integral_errorL += (errorL * 0.004);
  PcontrolL = Kp * errorL;
  IcontrolL = Ki * integral_errorL;
  DcontrolL = (Kd * (errorL - prev_errorL));
  PIDcontrolL = PcontrolL + IcontrolL + DcontrolL;
  vel_L += int((PIDcontrolL * 255) / 1080);
  analogWrite(EA, vel_L);
  prev_errorL = errorL;
}


void PID_R()
{
  int rightTargetRpm = convertSpeed2Rpm(rightTargetSpeed);
  errorR = rightTargetRpm - rpmR;
  integral_errorR += (errorR * 0.004);
  PcontrolR = Kp * errorR;
  IcontrolR = Ki * integral_errorR;
  DcontrolR = (Kd * (errorR - prev_errorR));
  PIDcontrolR = PcontrolR + IcontrolR + DcontrolR;
  vel_R += int((PIDcontrolR * 255) / 1080);
  analogWrite(EB, vel_R);
  prev_errorR = errorR;
}


void pulseCounterL()
{
  pulseCountL++;
  PID_L();
}

void pulseCounterR()
{
  pulseCountR++;
  PID_R();
}


//   function definitions
//------------------------------------------------
void speedSetup(int left, int right)
{
  leftTargetSpeed = left;
  rightTargetSpeed = right;
  analogWrite(EA, left);
  analogWrite(EB, right);
}

//direction set to move forward
void moveFront(int past_key)
{
  digitalWrite(A2, HIGH);
  digitalWrite(A1, LOW);
  digitalWrite(B4, HIGH);
  digitalWrite(B3, LOW);
  if(past_key == 7)
  {
    speedSetup(vel_L+50, vel_R+50);
    delay(500);
  }
  buf = past_key;
}

//direction set to move backward
void moveBack(int past_key)
{
  digitalWrite(A2, LOW);
  digitalWrite(A1, HIGH);
  digitalWrite(B4, LOW);
  digitalWrite(B3, HIGH);

  if(past_key == 7)
  {
    speedSetup(vel_L+50, vel_R+50);
    delay(500);
  }
  buf = past_key;
}


void Forward()
{
  speedSetup(vel_L, vel_R);
}


void Backward()
{
  speedSetup(vel_L, vel_R);
}


void LeftForward()
{
  speedSetup(vel_L, int(vel_R + 80));
}


void RightForward()
{
  speedSetup(int(vel_L + 80), vel_R);
}


void LeftBackward()
{
  speedSetup(vel_L, int(vel_R + 80));
}


void RightBackward()
{
  speedSetup(int(vel_L + 80), vel_R);
}


void Stop(int past_key)
{
  for(int i = (vel_L >= vel_R) ? vel_L:vel_R ; i>0 ; i-=10)
  {
      speedSetup(i, i);
      delay(500);
  }   
  buf = past_key;
}


//to keep the car's moving direction, get the previous driving method as a parameter
void SpeedUp(int past_key)
{
  vel_L += 10;
  vel_R += 10;

  if(vel_L > 250) vel_L = 250;
  if(vel_R > 250) vel_R = 250;
  
  switch(past_key)
  {
    case 1:
    case 2: speedSetup(vel_L, vel_R); break;
    case 3:
    case 4: speedSetup(vel_L, int(vel_R + 80)); break;
    case 5:
    case 6: speedSetup(int(vel_L + 80), vel_R); break;
    case 7: speedSetup(0, 0); break;
  }
}


//to keep the car's moving direction, get the previous driving method as a parameter
void SpeedDown(int past_key)
{
  vel_L -= 10;
  vel_R -= 10;

  if(vel_L < 40) vel_L = 40;
  if(vel_R < 40) vel_R = 40;
  
  switch(past_key)
  {
    case 1:
    case 2: speedSetup(vel_L, vel_R); break;
    case 3:
    case 4: speedSetup(vel_L, int(vel_R + 80)); break;
    case 5:
    case 6: speedSetup(int(vel_L + 80), vel_R); break;
    case 7: speedSetup(0, 0); break;
  }
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

  attachInterrupt(digitalPinToInterrupt(encoderL), pulseCounterL, RISING);
  attachInterrupt(digitalPinToInterrupt(encoderR), pulseCounterR, RISING);
  //attachInterrupt(digitalPinToInterrupt(encoderL_g), PID_L, RISING);
  //attachInterrupt(digitalPinToInterrupt(encoderR_g), PID_R, RISING);
   
  MsTimer2::set(500, calculateRpm);
  MsTimer2::start();
  Serial.begin(57600);
  if(CAN0.begin(CAN_500KBPS) == CAN_OK) Serial.print("CAN initialized\r\n");
  else Serial.print("CAN failed to initialize\r\n");

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
    duration = pulseIn(echo, HIGH);
    distance = ((float)(340 * duration) / 10000) / 2;
    Serial.print("distance : ");
    Serial.println(distance);

    ttc = (int)(distance / rpm);

    CAN0.readMsgBuf(&len, &can_rx_buffer);
    rxId = CAN0.getCanId();
    int msg = (int)can_rx_buffer;
    switch(msg)
    {
        case 1: moveFront(msg); Forward();       break;
        case 2: moveBack(msg);  Backward();      break;
        case 3: moveFront(msg); LeftForward();   break;
        case 4: moveFront(msg); RightForward();  break;
        case 5: moveBack(msg);  LeftBackward();  break;
        case 6: moveBack(msg);  RightBackward(); break;
        case 7: Stop(msg);      break;
        case 8: SpeedUp(buf);   break;
        case 9: SpeedDown(buf); break;
    }
}
