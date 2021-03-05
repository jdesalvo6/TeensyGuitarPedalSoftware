#define PIN_ENCODER_R 2
#define PIN_ENCODER_L 3
#define PIN_ENCODER_PUSH 33
 
void setup()
{
  // set pins as input with internal pull-up resistors enabled
  pinMode(PIN_ENCODER_L, INPUT_PULLUP);
  pinMode(PIN_ENCODER_R, INPUT_PULLUP);
  pinMode(PIN_ENCODER_PUSH, INPUT_PULLUP);
  digitalWrite(PIN_ENCODER_L, HIGH);
  digitalWrite(PIN_ENCODER_R, HIGH);
  //digitalWrite(PIN_ENCODER_PUSH, HIGH);
  Serial.begin(9600);
 
}

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
  if (digitalRead(PIN_ENCODER_PUSH) == LOW)
  {
    Serial.println("push");
    delay(300);
  }
}
