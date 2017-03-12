


#include <SerialCommand.h>


  // Arduino LED on board
 #define Enable 4  //High=disable  
 
#define trigger 6
#define anschlag 8
#define capsulePresent 9
#define STEP 10
#define DIR 11
#define klemme 12
#define arduinoLED 13 




SerialCommand SCmd;     // The  SerialCommand object
#include <Arduino.h>
#include "DRV8834.h"
#include "A4988.h"
#include "DRV8825.h"

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200

// All the wires needed for full functionality
#define DIR 11
#define STEP 10

// 2-wire basic config, microstepping is hardwired on the driver
// BasicStepperDriver stepper(DIR, STEP);

// microstep control for DRV8834
//#define M0 10
//#define M1 11
//DRV8834 stepper(MOTOR_STEPS, DIR, STEP, M0, M1);

// microstep control for A4988
#define MS1 10
#define MS2 11
#define MS3 12
A4988 stepper(MOTOR_STEPS, DIR, STEP, MS1, MS2, MS3);

// microstep control for DRV8825
// same pinout as A4988, different pin names, supports 32 microsteps
// #define MODE0 10
// #define MODE1 11
// #define MODE2 12
// DRV8825 stepper(MOTOR_STEPS, DIR, STEP, MODE0, MODE1, MODE2);


//////////////////////////////////////////////////////////////////////////
const float steptodeg = 360 / 200; // Xsteps equals 1 deg
int turntodeg = 200 ; // X deg on turn
const float maxrotate = 3000; //max rotation in deg
int dir = 1;
int rpm = 100;
int initrpm = 100;
int initstepps = 5;
const int stepping = 1;
int position = 0; // speicher position relativ zum Anschlag
int var;
int ablauf = 0;
int turntomuch = 0;
int turnmax = (maxrotate/turntodeg)*2;
bool capsulepresent=false;

//////////////////////////////////////////////////////////////////////////////////


void setup() {
pinMode(10, INPUT_PULLUP);
pinMode(11, INPUT_PULLUP);


pinMode(anschlag, INPUT);
  pinMode(anschlag, INPUT_PULLUP);

  pinMode(capsulePresent, INPUT);
  pinMode(capsulePresent, INPUT_PULLUP);

  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
 pinMode(trigger, OUTPUT);
  pinMode(arduinoLED, OUTPUT);
 pinMode(Enable, OUTPUT);
  pinMode(klemme, OUTPUT);
 
 pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  





  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);

  // Setup callbacks for SerialCommand commands
  SCmd.addCommand("turn",    turn1);          // Turns SRCR X°
  SCmd.addCommand("reset",    reset1);          // Stellt SRCR auf den uhrsprungszustand
  SCmd.addCommand("ok",    ansok);
  SCmd.addCommand("nok",    ansnot);
  SCmd.addCommand("present",    present);
  SCmd.setDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?")

  Serial.println("Readynew");
  /*
    while(digitalRead(anschlag) == LOW){
      stepper.setRPM(100);
      stepper.setMicrostep(stepping); // make sure we are in full speed mode
      stepper.rotate(-1);
     }
  */

  /*
         Set target motor RPM.
  */
  stepper.setRPM(rpm);
  stepper.setMicrostep(stepping); // make sure we are in full speed mode

}
bool stepperPositionOk = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {

  SCmd.readSerial();     // We don't do much, just process serial commands

  checkHome();
  checkCasule();
  switch (ablauf) {
    case 1:
      klemm();
      break; // dann 2

    case 2:
      trig();
      break; //dann 9

    case 3:// wird nur durch pc aufgerufen 
      ansok();
      break; // dann 4

      case 4:
      reset1();
      break; // dann 9

        case 5: // wird nur durch pc aufgerufen 
      ansnot();
      break; // dann 9
    case 9:
      donothing();
      break;
    case 99:
      capsuleReadErr();

      break; // dann 4

  }


}


void turn1() {
  if (stepperPositionOk)
  {
    digitalWrite(arduinoLED, HIGH);
    
    if (turntodeg > maxrotate) {
      turntodeg = maxrotate;
    }
 on();

    if (dir == 1 && (position + turntodeg) < maxrotate) var = 1;
    if (dir == 1 && (position + turntodeg) >= maxrotate) var = 2;

    if (dir == -1 && (position - turntodeg) > 0) var = 3;
    if (dir == -1 && (position - turntodeg) <= 0) var = 4;


    switch (var) {
      case 1:
        stepper.rotate( turntodeg * steptodeg);
        position += turntodeg ;
        break;
      case 2:
        stepper.rotate( (maxrotate - position)* steptodeg);
        position = maxrotate ;
        dir = -1;
        break;
      case 3:
        stepper.rotate( -turntodeg * steptodeg);
        position = position - turntodeg ;
        break;
      case 4:
        stepper.rotate( -(position)* steptodeg);
        position = 0 ;
        dir = 1;
        break;



    }

    digitalWrite(arduinoLED, LOW);

    Serial.println("turned");
    Serial.println(var);
    Serial.println(dir);
    Serial.println(position);
  }
  else {
    Serial.println("Error stepper not homed!");

  }
}


void reset1() {
capsulepresent=false;
  stepperPositionOk = false;
  ablauf=9;
  //Serial.println("reseted");

}


void initStepper() {
  //Serial.println("init!");

  stepper.setRPM(initrpm);
  stepper.setMicrostep(stepping); // make sure we are in full speed mode
  stepper.rotate(-initstepps);
}


void turnStepper(int stepps) {
  int step = 0;

  for (int speed = 100; step <= stepps; speed++) {

    stepper.setRPM(speed * 10);

    stepper.rotate(5 * steptodeg);
    step = +5;
  }
}


void checkHome() {
  if (!stepperPositionOk && digitalRead(anschlag) == HIGH) { //set to high to activate homing
    on();
    
 delay(0);
    initStepper();
    //notklemm();
    position = 0;
    dir = 1;
    
  }
  else if (!stepperPositionOk)
  {
    stepperPositionOk = true;
    stepper.setRPM(rpm);
    position = 0;
    dir = 1;
off();
    Serial.println("homed");
  }



}

void checkCasule() {
  if (!capsulepresent && stepperPositionOk && digitalRead(capsulePresent) == HIGH) {
    ablauf = 1;
    Serial.println("present");
    turntomuch = 0;
    delay(200);
    capsulepresent=true;
  }

}

void klemm() {
  on();

 delay(100);
  digitalWrite(klemme, HIGH);
  Serial.println("geklemmt");
  ablauf = 2;
}
void notklemm(){
   digitalWrite(klemme, LOW);
   
  Serial.println("notgeklemmt");
  ablauf = 9;
  
  }

void trig() {
  digitalWrite(trigger, HIGH);
  delay(100);
  digitalWrite(trigger, LOW);
  Serial.println("triggert");
  ablauf = 9;
}

void ansok() {
 
 if(capsulepresent==true){
  notklemm();
  //Send Capsule to Location
  Serial.println("OK");
ablauf=4;
 }
}
void ansnot() {
  turntomuch++;
  if (turntomuch <= turnmax && stepperPositionOk &&capsulepresent==true) {
    turn1();
    delay(100);
    trig();
    Serial.println("OK");
    ablauf = 9;
  }
  else if (stepperPositionOk &&capsulepresent==true){
    ablauf = 99;
  }
}

void capsuleReadErr() {
  notklemm();
  //Serial.println("capsuleReadErr");
//Send capsule to default location
ablauf=4;

}
void donothing() {
}
void present(){
  ablauf = 1;
   turntomuch = 0;
    delay(200);
    Serial.println("present");
  capsulepresent=true;
  
  
  }
void on(){
  digitalWrite(Enable, LOW);
  //Serial.print("on");
  }
void off(){
  digitalWrite(Enable, HIGH);
  //Serial.print("off");
  }

void unrecognized(const char* fail) {

  Serial.print("Error with: ");
  Serial.println(fail);

}




