#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3

//screen defines
#define TFT_DC      9
#define TFT_CS      37
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

//rotary encoder defines
#define PIN_ENCODER_R 2
#define PIN_ENCODER_L 3
#define PIN_ENCODER_PUSH 33
#define VOL_ENC_L 28
#define VOL_ENC_R 29
#define VOL_ENC_PUSH 99 //set this pin to a real value next time when working with the board

// startup settings for the effects
#define FLANGE_DELAY_LENGTH (3*AUDIO_BLOCK_SAMPLES)
float ampGain = 30;
float flangeOffset = FLANGE_DELAY_LENGTH/4;
float flangeDepth = FLANGE_DELAY_LENGTH/4;
float maxDepth = 165;
float flangeModFreq = 1;
float maxFreq = 3.5;
float masterVol = 0.8;
float fvDry = 0.4;
float fvWet = 0.9;
float fvRoomSize = 0.6;
float fvDamp = 0.4;

//setting on/off logic
bool fvOn = false;
bool distOn = false;
bool flangeOn = false;
int activeEff = 0;

//initialization for the effect user input array
int activeEffect;
const byte numChars = 10;
char EffArr[numChars];
int numEff;

// GUItool: begin automatically generated code
AudioControlSGTL5000     sgtl5000_1;
AudioInputI2S            lineIn;         
AudioEffectFreeverb      freeverbBlock;     
AudioEffectFlange        flangeBlock;       
AudioMixer4              mixFVOut;        
AudioAmplifier           ampBlock;        
AudioMixer4              outMix;        
AudioOutputI2S           lineOut;     
AudioConnection          fvInCord(lineIn, 1, freeverbBlock, 0);
AudioConnection          distInCord(lineIn, 1, ampBlock, 0);
AudioConnection          flangeInCord(lineIn, 1, flangeBlock, 0);
AudioConnection          fvDryCord(lineIn, 1, mixFVOut, 1);
AudioConnection          fvMixCord(freeverbBlock, 0, mixFVOut, 0);
AudioConnection          flangeOutCord(flangeBlock, 0, outMix, 2);
AudioConnection          fvOutCord(mixFVOut, 0, outMix, 0);
AudioConnection          distOutCord(ampBlock, 0, outMix, 1);
AudioConnection          outputCord(outMix, 0, lineOut, 1);     
AudioConnection          bypassCord(lineIn, 1, outMix, 3); //use this for bypass
// GUItool: end automatically generated code

void setup() 
{
  // generic setup
  Serial1.begin(9600); //rx1 and tx1 = pins 0 and 1 on Teensy for BT
  Serial.begin(9600);
  delay(500);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setFont(Arial_16);
  delay(250);
  AudioMemory(524);

  //disconnect effects at first
  flangeOutCord.disconnect();
  fvOutCord.disconnect();
  distOutCord.disconnect();
  //bypassCord.disconnect();

  //encoder setup
  pinMode(PIN_ENCODER_L, INPUT_PULLUP);
  pinMode(PIN_ENCODER_R, INPUT_PULLUP);
  pinMode(PIN_ENCODER_PUSH, INPUT_PULLUP);
  digitalWrite(PIN_ENCODER_L, HIGH);
  digitalWrite(PIN_ENCODER_R, HIGH);
  pinMode(VOL_ENC_L, INPUT_PULLUP);
  pinMode(VOL_ENC_R, INPUT_PULLUP);
  pinMode(VOL_ENC_PUSH, INPUT_PULLUP);
  digitalWrite(VOL_ENC_L, HIGH);
  digitalWrite(VOL_ENC_R, HIGH);
  
  // setup SGTL5000 and master audio
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(masterVol);
  sgtl5000_1.muteHeadphone();
  sgtl5000_1.unmuteLineout();
  outMix.gain(0, masterVol);

  // flange setup
  short delayline[FLANGE_DELAY_LENGTH];   
  flangeBlock.begin(delayline, FLANGE_DELAY_LENGTH, flangeOffset, flangeDepth, flangeModFreq);

  // Amp setup
  ampBlock.gain(ampGain);

  // freeverb setup
  mixFVOut.gain(0, fvWet); // "wet" signal
  mixFVOut.gain(1, fvDry); // "dry" signal
  freeverbBlock.roomsize(fvRoomSize);
  freeverbBlock.damping(fvDamp);

  tft.println("3 Effect Demo - Backupzz");
}

void loop() 
{
  MasterVol();
  BTCommandCheck();
  Bypass();
}

void Bypass() //disconnect all effects and just connect the dry in/out cord
{
   if (digitalRead(VOL_ENC_PUSH) == LOW)
   {
     flangeOutCord.disconnect();
     fvOutCord.disconnect();
     distOutCord.disconnect();
     bypassCord.connect();
   }
  
}

void MasterVol()
{
  if (digitalRead(VOL_ENC_L) == LOW && masterVol >= 0)
  {
    Serial.println("left");
    masterVol = masterVol - 0.05;
    Serial.println(masterVol);
    outMix.gain(0, masterVol);
    delay(75);
  }
  if (digitalRead(VOL_ENC_R) == LOW && masterVol <= 0.95)
  {
    Serial.println("right");
    masterVol = masterVol + 0.05;
    Serial.println(masterVol);
    outMix.gain(0, masterVol);
    delay(75);
  }
}

void BTCommandCheck()
{
  // create a variable called active effect and set that to whatever is active
  // when that is active, use if statement to determine which effect gets changed
 char inChar;
 if(Serial1.available() > 0) // use for BT
 {
   inChar = (char)Serial1.read();
   if(Serial1.availableForWrite())
   {
      if (inChar == '1')
      {
        Selector(); //was orderer
      }
      else if (inChar == '2')
      {
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(0, 8);
      }
      else if (inChar == '3')
      {
        effectEditor();
      }
   }
 }
}

void Selector() //int numEff
{
    // loop variable initialization
    static byte ndx = 0;
    char endMarker = 'z';
    char rc;
    int cnt = 0;
    numEff = 0;

    tft.println("Enter Effects to toggle"); //the logic behind the loop is that the user can enter up to 10 effects MAX.
    // when the user sends a 'z' it is like they are hitting 'done' in the app
    while (cnt <= 10)
    {
       while (Serial1.available()==0) {}
       rc = Serial1.read();
       if (rc != endMarker) 
       {
          EffArr[ndx] = rc;
          ndx++;
          cnt++;
       }
       else if (rc == 'z') 
       {
          EffArr[ndx] = 'z'; // terminate the string
          numEff = cnt;
          tft.print("numEff = ");
          tft.println(numEff);
          ndx = 0;
          cnt = 0;
          break;
       }
       tft.println("Enter another Effect?");
    }
    Connections();
}

void Connections()
{
  bypassCord.disconnect();
  bool breaker = false;
  for (int i = 0; i <= 9; i++)
  {
    
    switch(EffArr[i])
    {
      case '1': //let 1 be flange
        if (!flangeOn)//off turning on
        {
          tft.println("Flange on");
          flangeOutCord.connect();
          flangeOn = true;
          activeEff = activeEff + 1;
        }
        else //on turning off
        {
          tft.println("Flange off");
          flangeOutCord.disconnect();
          flangeOn = false;
          activeEff = activeEff - 1;
        }
        break;
      case '2': //let 2 be amp
        if (!distOn) //off  turning on
        {
          tft.println("distortion on");
          distOutCord.connect();
          distOn = true;
          activeEff = activeEff + 1;
        }
        else //on  turning off
        {
          tft.println("distortion off");
          distOutCord.disconnect();
          distOn = false;
          activeEff = activeEff - 1;
        }
        break;
      case '3': //let 3 be freeverb
        //code here
        if (!fvOn) //off  turning on
        {
          tft.println("reverb on");
          fvOutCord.connect();
          fvOn = true;
          activeEff = activeEff + 1;
        }
        else //on  turning off
        {
          tft.println("reverb off");
          fvOutCord.disconnect();
          fvOn = false;
          activeEff = activeEff - 1;
        }
        break;
      case 'z': //terminating case
        if (i == 0)
        {
          breaker = true;
          tft.println("Creation cancelled");
          break;
        }
        else
        {
          breaker = true;
          tft.println("finished toggling");
        }
    }
    if (breaker == true) //if the terminating character has been reached, just break out of the loop
    {
      break;
    }
  }
  tft.println("done with connections");
}

void effectEditor()
{
  tft.println("Which effect do you want to change");
  char rc;
  char param; //the param that you are changing

  while (Serial1.available()==0) {}
  rc = Serial1.read();
  if (rc == '1')
  {
    //do stuff for flange, at this point probably have to select what you want changed
    while (Serial1.available()==0) {}
    param = Serial1.read();
    paramChanger(rc, param);
  }
  else if (rc == '2')
  {
    //do stuff for amp, only has 1 param (volume) for now
    while (Serial1.available()==0) {}
    param = Serial1.read();
    paramChanger(rc, param);
  }
  else if (rc == '3')
  {
    //do stuff for reverb, select which param you are changing
    tft.print("heyo");
    while (Serial1.available()==0) {}
    param = Serial1.read();
    tft.print("heyo2");
    paramChanger(rc, param);
  }
  else if (rc == 'z')
  {
    tft.println("Effect paramenter change cancelled.");
  }
}

void paramChanger(char eff, char param)
{
  bool looper = true;
  if (eff == '1')
  {
    tft.println("editing flange effect.");
    if (param == '1') //this one doesn't sound like it affects it that much. maybe just take it out
    {
      Serial.println("flangeOffset");
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW)
        {
          Serial.println("left");
          flangeOffset = flangeOffset - 10;
          Serial.println(flangeOffset);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW)
        {
          Serial.println("right");
          flangeOffset = flangeOffset + 10;
          Serial.println(flangeOffset);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          flangeBlock.voices(flangeOffset, flangeDepth, flangeModFreq);
          looper = false;
          delay(300);
        }
      }
    }
    else if (param == '2')
    {
      Serial.println("flangeDepth");
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW && flangeDepth > 0)
        {
          Serial.println("left");
          flangeDepth = flangeDepth - 10;
          Serial.println(flangeDepth);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW && flangeDepth < maxDepth)
        {
          Serial.println("right");
          flangeDepth = flangeDepth + 10;
          Serial.println(flangeDepth);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          flangeBlock.voices(flangeOffset, flangeDepth, flangeModFreq);
          looper = false;
          delay(300);
        }
      }
    }
    else if (param == '3')
    {
      Serial.println("freqMod");
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW && flangeModFreq > 0.1)
        {
          Serial.println("left");
          flangeModFreq = flangeModFreq - 0.1;
          Serial.println(flangeModFreq);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW && flangeModFreq < maxFreq)
        {
          Serial.println("right");
          flangeModFreq = flangeModFreq + 0.1;
          Serial.println(flangeModFreq);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          flangeBlock.voices(flangeOffset, flangeDepth, flangeModFreq);
          looper = false;
          delay(300);
        }
      }
    }
  }
  else if (eff == '2')
  {
    tft.println("editing amp effect.");
    if (param == '1')
    {
      while(looper) //loop so the user can keep changing the effect
      {
        if (digitalRead(PIN_ENCODER_L) == LOW)
        {
          Serial.println("left");
          ampGain = ampGain - 5;
          Serial.println(ampGain);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW)
        {
          Serial.println("right");
          ampGain = ampGain + 5;
          Serial.println(ampGain);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          ampBlock.gain(ampGain);
          looper = false;
          delay(300);
        }
      }
    }
  }
  else if (eff == '3')
  {
    tft.println("editing reverb effect.");
    if (param == '1')
    {
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW && fvDry >= 0)
        {
          Serial.println("left");
          fvDry = fvDry - 0.05;
          Serial.println(fvDry);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW && fvDry <= 1)
        {
          Serial.println("right");
          fvDry = fvDry + 0.05;
          Serial.println(fvDry);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          mixFVOut.gain(1, fvDry);
          looper = false;
          delay(300);
        }
      }
    }
    else if (param == '2')
    {
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW && fvWet >= 0)
        {
          Serial.println("left");
          fvWet = fvWet - 0.05;
          Serial.println(fvWet);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW && fvWet <= 1)
        {
          Serial.println("right");
          fvWet = fvWet + 0.05;
          Serial.println(fvWet);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          mixFVOut.gain(0, fvWet);
          looper = false;
          delay(300);
        }
      }
    }
    else if (param == '3')
    {
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW && fvWet >= 0)
        {
          Serial.println("left");
          fvRoomSize = fvRoomSize - 0.05;
          Serial.println(fvRoomSize);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW && fvWet <= 1)
        {
          Serial.println("right");
          fvRoomSize = fvRoomSize + 0.05;
          Serial.println(fvRoomSize);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          freeverbBlock.roomsize(fvRoomSize);
          looper = false;
          delay(300);
        }
      }
    }
    else if (param == '4')
    {
      while(looper)
      {
        if (digitalRead(PIN_ENCODER_L) == LOW && fvWet >= 0)
        {
          Serial.println("left");
          fvDamp = fvDamp - 0.05;
          Serial.println(fvDamp);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_R) == LOW && fvWet <= 1)
        {
          Serial.println("right");
          fvDamp = fvDamp + 0.05;
          Serial.println(fvDamp);
          delay(75);
        }
        if (digitalRead(PIN_ENCODER_PUSH) == LOW)
        {
          freeverbBlock.damping(fvDamp);
          looper = false;
          delay(300);
        }
      }
    }
  }
  tft.println("done editing params.");
}
