/*
====================================================
Tinyswitch - v1.0 - 11-07-2024 - paalsma@gmail.com
====================================================

Tiny switch decoder based on the ATTINY85 in conjuction with a LD293D motor driver for 
usage on a single switch/turnout.

At boot decoder led will flash address info (if led setting is on), this is always 3 numbers with 
a small pause in between. A slow blink is a 0. Count fast blinks for other numbers. 
Send any command to switch address 255 to have all decoders blink their address (even if led setting is off).

CV's
====

CV1: Address from 1 - 254, default 9 
CV2: Mirror throw and close (1 = normal, 2 = mirror), default 1
CV3: Led signal on boot (number), throw (double) & close (single), (1 = no, 2 = yes), default 1
CV4: Pulse duration between 10 - 100ms in 10ms steps, value 1 - 10, default 5 (50ms)

On track programming / functions
================================
1) Close switch
2) Throw switch X number of times (with interval < 2 seconds), led will turn on at X = 4
3) Close switch to activate programming/function

X  Function
===========
4  Show address (three numbers)
5  Change address
6  



Connections
===========

ATTINY85
--------
p1 (PB5) - Empty
p2 (PB3) - Status led
p3 (PB4) - LD293D p15 (IN4)
p4 (GND) - GND
p5 (PB0) - LD293D p9 (EN2)
p6 (PB1) - LD293D p10 (IN3)
p7 (PB2) - DCC signal  
p8 (VCC) - 5V


*/


#include <NmraDcc.h>
#include <EEPROM.h>

// declare DCC object
NmraDcc Dcc; 

// CV with factory defaults
// switch address
#define CV_ACCESSORY_DECODER_ADDRESS          1
#define CV_ACCESSORY_DECODER_ADDRESS_DEFAULT  9
// standard or mirrored (1 standard/2 mirror)
#define CV_ACCESSORY_DECODER_MIRROR           2
#define CV_ACCESSORY_DECODER_MIRROR_DEFAULT   1
// led on or off during boot & switching (1 no/2 yes)
#define CV_ACCESSORY_DECODER_LED              3
#define CV_ACCESSORY_DECODER_LED_DEFAULT      1
// pulse length (*10 ms)
#define CV_ACCESSORY_DECODER_PULSE            4
#define CV_ACCESSORY_DECODER_PULSE_DEFAULT    5
// value = 8 or 255, "factory reset"
#define CV_ACCESSORY_DECODER_RESET            8

// Globals for CV settings
uint8_t setting_address;
uint8_t setting_mirror;
uint8_t setting_led;
uint8_t setting_pulse;

// Counters for track program mode
uint8_t throwCount = 0;
uint32_t throwTime;
uint8_t program = 0;
bool programRunning = false;

// Programs (change progs correspond with CV address)
#define P_NONE            0

#define P_CHANGEADDRESS   1
#define P_CHANGEMIRROR    2
#define P_CHANGELED       3
#define P_CHANGEPULSE     4

#define P_SHOWADDRESS     5
#define P_SHOWMIRROR      6
#define P_SHOWLED         7
#define P_SHOWPULSE       8

// LD293D motor driver
const uint8_t EN1_PIN = 0;
const uint8_t IN1_PIN = 1;
const uint8_t IN2_PIN = 4;

// DCC sig pin
const uint8_t DCC_PIN = 2;

// status led
const uint8_t LED_PIN = 3;

// Called by DCC library after a CV change
void notifyCVChange(uint16_t CV, uint8_t Value)
{
  uint8_t oldValue = Value;

  // Show user a CV has been written
  fastBlink();

  if (CV == CV_ACCESSORY_DECODER_RESET && (Value == 8 || Value == 255)) {
    resetCVFactoryDefault();
    return;
  }

  // Check CV's
  Value = checkCV(CV, Value);

  // New CV not correct, write corrected value
  if (oldValue != Value) {
    _writeCV(CV, Value);
  }
}

// Check if new CV are ok, if not correct them
uint8_t checkCV(uint16_t CV, uint8_t Value)
{
  switch(CV) {
    case CV_ACCESSORY_DECODER_ADDRESS : 
      Value = (Value > 0 && Value <= 254) ? Value : CV_ACCESSORY_DECODER_ADDRESS_DEFAULT; 
      setting_address = Value; 
      break;
    case CV_ACCESSORY_DECODER_MIRROR  : 
      Value = Value > 1 ? 2 : Value; 
      setting_mirror = Value; 
      break;
    case CV_ACCESSORY_DECODER_LED     : 
      Value = Value > 1 ? 2 : Value; 
      setting_led = Value; 
      break;
    case CV_ACCESSORY_DECODER_PULSE   : 
      Value = Value > 10 ? 10 : max(Value, 1); 
      setting_pulse = Value; 
      break;
  }

  return Value;
}

// Combine wait and write (underscore because function exists)
void _writeCV(uint16_t CV, uint8_t Value)
{
  // I do not know if this is required before every write (todo), better safe then sorry
  while(!Dcc.isSetCVReady()) {}

  Dcc.setCV(CV, Value);
}

// Reset all CV's to factory default
void resetCVFactoryDefault()
{
  _writeCV(CV_ACCESSORY_DECODER_ADDRESS, CV_ACCESSORY_DECODER_ADDRESS_DEFAULT);
  _writeCV(CV_ACCESSORY_DECODER_MIRROR, CV_ACCESSORY_DECODER_MIRROR_DEFAULT);
  _writeCV(CV_ACCESSORY_DECODER_LED, CV_ACCESSORY_DECODER_LED_DEFAULT);
  _writeCV(CV_ACCESSORY_DECODER_PULSE, CV_ACCESSORY_DECODER_PULSE_DEFAULT);
  _writeCV(CV_ACCESSORY_DECODER_RESET, 0);

  setting_address = CV_ACCESSORY_DECODER_ADDRESS_DEFAULT;
  setting_mirror = CV_ACCESSORY_DECODER_MIRROR_DEFAULT;
  setting_led = CV_ACCESSORY_DECODER_LED_DEFAULT;
  setting_pulse = CV_ACCESSORY_DECODER_PULSE_DEFAULT;
}


// Called from DCC library when a turnout is set
void notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower) 
{
  // Ignore pulselength packet
  if( OutputPower == 0 ) return;   

  // Any command on address 255 will cause all tiny decoders to flash
  if (Addr == 255) {
    showNumber(setting_address);
    return;
  }

  // Check if our address needs to process anything
  if (Addr == setting_address) { 

    // We are in programming mode
    if (programRunning) {
      if (Direction) {
        // Setting value +1
        throwCount++;
        throwTime = millis();

        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
        // store new value
        throwCount = checkCV(program, throwCount);
        _writeCV(program, throwCount);

        // Show user a CV has been written  
        fastBlink();

        // Done!
        program = P_NONE;
        programRunning = false;
        throwCount = 0;
      }

      return;
    }

    // Normal operation, throw or close
    if (Direction) {
      throwSwitch();
      throwCountCheck();
    } else {
      closeSwitch();
      closeCountCheck();
    }
  }
}

void setup()
{    
  // Init pins
  pinMode(EN1_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  pinMode(DCC_PIN, INPUT_PULLUP);
  
  pinMode(LED_PIN, OUTPUT);

  // Init DCC object
  Dcc.pin(0, DCC_PIN, 1);
  Dcc.init(MAN_ID_DIY, 10, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE, 0);

  // First run? Set all CV's to default in EEPROM
  if (Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS) == 255 || Dcc.getCV(CV_ACCESSORY_DECODER_RESET) == 255) {
    resetCVFactoryDefault();

    fastBlink();
  }

  // Get the settings from the EEPROM CV's
  setting_address = Dcc.getCV(CV_ACCESSORY_DECODER_ADDRESS);
  setting_mirror = Dcc.getCV(CV_ACCESSORY_DECODER_MIRROR);
  setting_led = Dcc.getCV(CV_ACCESSORY_DECODER_LED);
  setting_pulse = Dcc.getCV(CV_ACCESSORY_DECODER_PULSE);

  // Show decoder address
  if (setting_led > 1) {
    showNumber(setting_address);
  }
}

void loop()
{
  // Do all DCC stuff in library
  Dcc.process();

  // Initiated a program on track sequence, check if it hasn't timed out
  checkTimer();

  // Switch to running a specific program 
  if (programRunning) {
    startProgram();
  }
}

// Does what it says
void closeSwitch() 
{
  // Set motor driver
  if (setting_mirror == 1) {
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
  } else {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
  }
  
  // Enable switch motor for setting_setting_pulse * 10 ms
  digitalWrite(EN1_PIN, HIGH);
  delay(setting_pulse * 10);
  digitalWrite(EN1_PIN, LOW);

  if (setting_led == 1) {
    return;
  }

  // Blink led once as confirmation
  blink();
}

// Enter a program mode or just reset counter
void closeCountCheck()
{
    throwCount = 0;
    throwTime = 0;

    if (program) {
        programRunning = true;
    }
}

// Does what it says :)
void throwSwitch() 
{
  // Set motor driver
  if (setting_mirror == 1) {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
  } else {
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
  }
  
  // Enable switch motor for pulse * 10 ms
  digitalWrite(EN1_PIN, HIGH);
  delay(setting_pulse * 10);
  digitalWrite(EN1_PIN, LOW);

  if (setting_led == 1 && program == P_NONE) {
    return;
  }
  
  // Blink led twice as confirmation
  blink();
  blink();
}

// Check if user wants to enter a program mode
// (switch thrown > 3 times within interval)
void throwCountCheck() {
    throwCount++;
    throwTime = millis();

    switch(throwCount) {
      case 4  : program = P_SHOWADDRESS; break;
      case 5  : program = P_CHANGEADDRESS; break;
      case 6  : program = P_SHOWMIRROR; break;
      case 7  : program = P_CHANGEMIRROR; break;
      case 8  : program = P_SHOWLED; break;
      case 9  : program = P_CHANGELED; break;
      case 10 : program = P_SHOWPULSE; break;
      case 11 : program = P_CHANGEPULSE; break;
      default : program = P_NONE; break;
    }

    if (throwCount > 11) {
      throwCount = 0;
    }

    if (program) {
      digitalWrite(LED_PIN, HIGH);
    }
}

// Check if time since last command > 4s
void checkTimer() 
{
  if (!throwTime) {
    return;
  }

  if (millis() - throwTime > 4000) {
    digitalWrite(LED_PIN, LOW);

    throwCount = 0;
    throwTime = 0;
    program = P_NONE;
  }
}

void startProgram()
{
  if (!programRunning) {
    return;
  }

  fastBlink();

  if (program == P_SHOWADDRESS) {
    showNumber(setting_address);
    program = P_NONE;
    programRunning = false;
    throwCount = 0;
  }

  if (program == P_SHOWMIRROR) {
    showSingleNumber(setting_mirror);
    program = P_NONE;
    programRunning = false;
    throwCount = 0;
  }

  if (program == P_SHOWLED) {
    showSingleNumber(setting_led);
    program = P_NONE;
    programRunning = false;
    throwCount = 0;
  }

  if (program == P_SHOWPULSE) {
    showSingleNumber(setting_pulse);
    program = P_NONE;
    programRunning = false;
    throwCount = 0;
  }

  if (program == P_CHANGEADDRESS || program == P_CHANGELED || program == P_CHANGEMIRROR || program == P_CHANGEPULSE) {
    digitalWrite(LED_PIN, HIGH);
    throwCount = 0;
  }
  
}

void blink() 
{
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(75);
}

void fastBlink()
{
  for(uint8_t i = 0; i < 15; i ++) {
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }

  delay(1000);
}

void showNumber(uint8_t n) 
{
  uint8_t x;
  
  x = n / 100;
  
  showSingleNumber(x);

  n = n - (x * 100);
  x = n / 10;

  showSingleNumber(x);

  n = n - (x * 10);

  showSingleNumber(n);
}

void showSingleNumber(uint8_t n) 
{
  if (n) {
    for(uint8_t i = 0; i < n; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(150);
    }
  } else {
      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      delay(150);
  }

  delay(850);
}