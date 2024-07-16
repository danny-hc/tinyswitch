#include <NmraDcc.h>
#include <EEPROM.h>

NmraDcc Dcc; 

const int myAddress = 20;
const byte DCC_PIN = 2;
const byte pulse = 50;


void notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower)
{
  if( OutputPower == 0 ) return;

  if (Addr == myAddress) {    
    if (Direction) {
      throwSwitch();
    } else {
      closeSwitch();
    }
  }
}

void setup()
{    
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  for(byte i = 0; i < 10; i++) {
    blink();
  }

  Dcc.pin(0, DCC_PIN, 1);
  Dcc.init(MAN_ID_DIY, 10, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE, 0);
}

void loop()
{
    Dcc.process();
}

void closeSwitch() {
  digitalWrite(1, HIGH);
  digitalWrite(4, LOW);
  
  digitalWrite(0, HIGH);
  delay(pulse);
  digitalWrite(0, LOW);

  blink();
}

void throwSwitch() {
  digitalWrite(1, LOW);
  digitalWrite(4, HIGH);
  
  digitalWrite(0, HIGH);
  delay(pulse);
  digitalWrite(0, LOW);
  
  blink();
  blink();
}

void blink() {
  digitalWrite(3, HIGH);
  delay(100);
  digitalWrite(3, LOW);
  delay(100);
}