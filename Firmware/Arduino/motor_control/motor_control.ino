#include <ros.h>
#include <std_msgs/Int32.h>

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
//-----------------------------------------------

#define EA_R   2
#define A1_R  53
#define A2_R  51

#define EB_R  10
#define B3_R  43
#define B4_R  41

#define EA_L   8
#define A1_L  22
#define A2_L  24

#define EB_L   3
#define B3_L   4
#define B4_L   5


//   Basic declaration to use rosserial
//------------------------------------------------
ros::NodeHandle  nh;
int buf;



void speedSetup(int vel_L, int vel_R)
{
  analogWrite(EA_L, vel_L);
  analogWrite(EA_R, vel_R);
  analogWrite(EB_R, vel_R);
  analogWrite(EB_L, vel_L);
}



// motor driver calibration
void Forward()
{
  speedSetup(140, 140);
  
  digitalWrite(A1_L, HIGH);
  digitalWrite(A2_L, LOW);
  
  digitalWrite(B3_L, HIGH);
  digitalWrite(B4_L, LOW);

  digitalWrite(A1_R, LOW);
  digitalWrite(A2_R, HIGH);

  digitalWrite(B3_R, LOW);
  digitalWrite(B4_R, HIGH);
}



//  motor driver calibration
void Backward()
{
  speedSetup(140, 140);
  
  digitalWrite(A1_L, LOW);
  digitalWrite(A2_L, HIGH);
  
  digitalWrite(B3_L, LOW);
  digitalWrite(B4_L, HIGH);

  digitalWrite(A1_R, HIGH);
  digitalWrite(A2_R, LOW);

  digitalWrite(B3_R, HIGH);
  digitalWrite(B4_R, LOW);
}



void LeftForward()
{
  Forward();
  speedSetup(130, 210);
}



void RightForward() {
  Forward();
  speedSetup(210, 130);
}



void LeftBackward()
{
  Backward();
  speedSetup(130, 210);
}



void RightBackward()
{
  Backward();
  speedSetup(210, 130);
}



void Stop()
{
  speedSetup(0, 0);
}



//   Subscriber function
//------------------------------------------------
void messageCb(const std_msgs::Int32& msg) {

  switch(msg.data){
    case 1: Forward();       buf = msg.data; break;
    case 2: Backward();      buf = msg.data; break;
    case 3: LeftForward();   buf = msg.data; break;
    case 4: RightForward();  buf = msg.data; break;
    case 5: LeftBackward();  buf = msg.data; break;
    case 6: RightBackward(); buf = msg.data; break;
    case 7: Stop();          buf = msg.data; break;
  }
  
}
ros::Subscriber<std_msgs::Int32> sub("toArduino", messageCb);


std_msgs::Int32 int_msg;

//     Publisher registration part
//----------------------------------------------
ros::Publisher chatter("chatter", &int_msg);



//  1. Setup all pins as output
//  2. Push and pull topics
//----------------------------------------------
void setup()
{
  Serial.begin(57600);
  nh.initNode();
  nh.advertise(chatter);
  nh.subscribe(sub);

  speedSetup(0, 0);
  
  pinMode(B4_R, OUTPUT);
  pinMode(B3_R, OUTPUT);

  pinMode(A1_L, OUTPUT);
  pinMode(A2_L, OUTPUT);
  
  pinMode(A2_R, OUTPUT);
  pinMode(A1_R, OUTPUT);

  pinMode(B3_L, OUTPUT);
  pinMode(B4_L, OUTPUT);
}


//   Publish received data from Raspberry pi
//---------------------------------------------
void loop()
{    
  int_msg.data = buf;
  chatter.publish(&int_msg);
  nh.spinOnce();
  delay(500);
}
