#include <math.h>
#include <MsTimer2.h>

#define encoderR 18
#define encoderL 21

#define EA 6
#define A1 12
#define A2 11

#define B3 7
#define B4 9
#define EB 8

#define front_echo 33
#define front_trig 37

float reflect_duration, obstacle_distance, velocity, ttc;
float system_delay = 0.5;
const float max_vel = 250;
const float ppr = 1800;
volatile float pulseCountL = 0, pulseCountR = 0;
volatile int rpmL, rpmR, rpm;

void print_status()
{
    Serial.print("distance : ");
    Serial.println(obstacle_distance);
//    Serial.print("rpmL : ");
//    Serial.println(rpmL);
//    Serial.print("rpmR : ");
//    Serial.println(rpmR);
    Serial.print("velocity : ");
    Serial.println(velocity);
    Serial.print("ttc : ");
    Serial.println(ttc);
}

float calculate_ttc()
{
  float _ttc;
  velocity = ((float)rpm * 6.6 * 3.141592) / 60.0;
  _ttc = (obstacle_distance / velocity);
  if(_ttc < 0) _ttc = 0;
  return _ttc;
}

void speedSetup(int left, int right)
{
    analogWrite(EA, left);
    analogWrite(EB, right);
}

float detect_distance()
{
    float _distance;
    digitalWrite(front_trig, LOW);
    delayMicroseconds(2);
    digitalWrite(front_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(front_trig, LOW);
    reflect_duration = pulseIn(front_echo, HIGH);
    _distance = ((float)(340 * reflect_duration) / 10000) / 2;
    return _distance;
}

void aeb_handler()
{
  ttc = calculate_ttc();
  if(ttc <= (1.5 + system_delay))
  {
      speedSetup(0, 0);
      while(1)
      {
        obstacle_distance = detect_distance();
        if(obstacle_distance > 40) 
        {
          break;
        }
      }
  }
}

void speedCalibration()
{
    rpmL = (int)((pulseCountL / ppr) * (60.0 / 0.5));
    rpmR = (int)((pulseCountR / ppr) * (60.0 / 0.5));
    rpm = (rpmL + rpmR) / 2;
    
    pulseCountL = 0;
    pulseCountR = 0;
}

void pulseCounterL() { pulseCountL++; }
void pulseCounterR() { pulseCountR++; }

void setup()
{
    pinMode(encoderR, INPUT);
    pinMode(EA, OUTPUT);
    pinMode(A2, OUTPUT);
    pinMode(A1, OUTPUT);
               
    pinMode(encoderL, INPUT);
    pinMode(EB, OUTPUT);
    pinMode(B3, OUTPUT);
    pinMode(B4, OUTPUT);

    pinMode(front_trig, OUTPUT);
    pinMode(front_echo, INPUT);
    
    attachInterrupt(digitalPinToInterrupt(encoderL), pulseCounterL, RISING);
    attachInterrupt(digitalPinToInterrupt(encoderR), pulseCounterR, RISING);
           
    MsTimer2::set(500, speedCalibration);
    MsTimer2::start();

    digitalWrite(A2, HIGH);
    digitalWrite(A1, LOW);
    digitalWrite(B4, HIGH);
    digitalWrite(B3, LOW);

    Serial.begin(57600);
}

void loop()
{
  obstacle_distance = detect_distance();
  if(obstacle_distance > 80)
  {
    speedSetup(max_vel, max_vel);
  }
  else if(obstacle_distance < 80 and obstacle_distance >= 60)
  {
    speedSetup((max_vel / 25) * 17, (max_vel / 25) * 17);
  }
  else if(obstacle_distance < 60 and obstacle_distance >= 40)
  {
    speedSetup((max_vel / 25) * 13, (max_vel / 25) * 13);
  }
  else
  {
    speedSetup((max_vel / 25) * 9, (max_vel / 25) * 9);
    aeb_handler();
  }
  
  print_status();
}
