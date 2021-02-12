#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=152.0056915283203,414.8238525390625
AudioEffectFlange        flange1;        //xy=431.0056610107422,590.823823928833
AudioEffectDelay         delay1;         //xy=561.0056190490723,431.8238067626953
AudioMixer4              mixer1;         //xy=569.0056762695312,263.8238468170166
AudioMixer4              mixer2;         //xy=781.0056419372559,570.8238258361816
AudioOutputI2S           i2s2;           //xy=1146.0056915283203,558.8238525390625
AudioConnection          patchCord1(i2s1, 1, flange1, 0);
AudioConnection          patchCord2(i2s1, 1, mixer2, 1);
AudioConnection          patchCord3(i2s1, 1, mixer1, 0);
AudioConnection          patchCord4(flange1, 0, mixer2, 2);
AudioConnection          patchCord5(delay1, 0, mixer1, 1);
AudioConnection          patchCord6(delay1, 0, mixer2, 0);
AudioConnection          patchCord7(mixer1, delay1);
AudioConnection          patchCord8(mixer2, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=369.00569915771484,706.8238468170166
// GUItool: end automatically generated code


const int buttonPinFlange = 28;
const int buttonPinDelay = 29;
Bounce pushbuttonFlange = Bounce(buttonPinFlange, 1);
Bounce pushbuttonDelay = Bounce(buttonPinDelay, 1);

void setup() {
  // put your setup code here, to run once:

  patchCord1.disconnect();
  patchCord2.disconnect();
  patchCord3.disconnect();
  
  delay(250);
  AudioMemory(524);
  Serial.begin(9600);
  pinMode(buttonPinFlange, INPUT_PULLUP);
  pinMode(buttonPinDelay, INPUT_PULLUP);

  //setup SGTL5000
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  //sgtl5000_1.volume(0.3);
  sgtl5000_1.muteHeadphone();
  sgtl5000_1.unmuteLineout();

  short delayline[5*AUDIO_BLOCK_SAMPLES];   
  flange1.begin(delayline, 5*AUDIO_BLOCK_SAMPLES, (5*AUDIO_BLOCK_SAMPLES)/4, (5*AUDIO_BLOCK_SAMPLES)/4, 1);
  flange1.voices((5*AUDIO_BLOCK_SAMPLES)/4, (5*AUDIO_BLOCK_SAMPLES)/4, 1);

  mixer1.gain(0, 0.5);
  mixer1.gain(1, 0.7);

  delay1.delay(0, 300);
  
}

byte previousStateF = HIGH;         // what state was the button last time
byte previousStateD = HIGH;
unsigned int countF = 0;            // how many times has it changed to low
unsigned int countD = 0; 
unsigned long countAtF = 0;         // when count changed
unsigned long countAtD = 0;
unsigned int countPrintedF = 0;
unsigned int countPrintedD = 0;
unsigned int mode = 0;

void loop()
{
  if (pushbuttonFlange.update()) 
  {
    if (pushbuttonFlange.fallingEdge())
    {
      countF = countF + 1;
      countAtF = millis();
    }
  }
  else
  {
    if (countF != countPrintedF)
    {
      unsigned long nowMillis = millis();
      if (nowMillis - countAtF > 100)
      {
        flangePress();
      }
    }
  }

  if (pushbuttonDelay.update()) 
  {
    if (pushbuttonDelay.fallingEdge())
    {
      countD = countD + 1;
      countAtD = millis();
    }
  }
  else
  {
    if (countD != countPrintedD)
    {
      unsigned long nowMillis = millis();
      if (nowMillis - countAtD > 100)
      {
        delayPress();
      }
    }
  }
  
}


void flangePress()
{
  if (mode != 1)
  {
    Serial.println("Flange Selected");
    countPrintedF = countF;
    patchCord1.connect();
    patchCord2.disconnect();
    patchCord3.disconnect();
    mode = 1;
  }
  else
  {
    Serial.println("Flange Already Selected");
    countPrintedF = countF;
  }
}

void delayPress()
{
  if (mode !=2)
  {
    Serial.println("Delay Selected");
    countPrintedD = countD;
    patchCord1.disconnect();
    patchCord2.connect();
    patchCord3.connect();
    mode = 2;
  }
  else
  {
    Serial.println("Delay Already Selected");
    countPrintedD = countD;
  }
}
