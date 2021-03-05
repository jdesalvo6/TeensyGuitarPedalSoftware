#include <Bounce.h>

#define PIN_ENCODER_L 3
#define PIN_ENCODER_R 2
 
void setup()
{
  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_L, INPUT_PULLUP);
  pinMode(PIN_ENCODER_R, INPUT_PULLUP);
  digitalWrite(PIN_ENCODER_L, HIGH);
  digitalWrite(PIN_ENCODER_R, HIGH);
  Serial.begin(9600);
 
}

unsigned int countF = 0;            // how many times has it changed to low
unsigned int countD = 0; 
unsigned long countAtF = 0;         // when count changed
unsigned long countAtD = 0;
unsigned int countPrintedF = 0;
unsigned int countPrintedD = 0;
unsigned int mode = 0;

void loop()
{
  if (digitalRead(PIN_ENCODER_L) == LOW)
  {
    Serial.println("left");
    delay(75);
  }
  if (digitalRead(PIN_ENCODER_R) == LOW)
  {
    Serial.println("right");
    delay(75);
  }
}
