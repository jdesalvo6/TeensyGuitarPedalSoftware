#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3

//screen defines
#define TFT_DC      9
#define TFT_CS      10 //37
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

//rotary encoder defines
#define PIN_ENCODER_R 5
#define PIN_ENCODER_L 3
#define PIN_ENCODER_PUSH 33
#define VOL_ENC_L 28
#define VOL_ENC_R 29
#define VOL_ENC_PUSH 25

// startup settings for the effects
#define FLANGE_DELAY_LENGTH (4*AUDIO_BLOCK_SAMPLES)
float flangeOffset = FLANGE_DELAY_LENGTH/4;
float flangeDepth = FLANGE_DELAY_LENGTH/4;
float flangeModFreq = 1;
float ampGain = 30;
float fvDry = 0.4;
float fvWet = 0.9;
float fvRoomSize = 0.6;
float fvDamp = 0.4;
float maxDepth = 165;
float maxFreq = 3.5;
float masterVol = 0.8;

//setting on/off logic
bool fvOn = false;
bool distOn = false;
bool flangeOn = false;
bool bypassOn = false;
int activeEff = 0;

//save states and their info
char currentEffect;
bool save1[3] = { false };
bool save2[3] = { false };
bool save3[3] = { false };
float paramsSave1[8] = {flangeOffset, flangeDepth, flangeModFreq, ampGain, 
                       fvDry, fvWet, fvRoomSize, fvDamp};
float paramsSave2[8] = {flangeOffset, flangeDepth, flangeModFreq, ampGain, 
                       fvDry, fvWet, fvRoomSize, fvDamp};
float paramsSave3[8] = {flangeOffset, flangeDepth, flangeModFreq, ampGain, 
                       fvDry, fvWet, fvRoomSize, fvDamp};

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
  distOutCord.disconnect(); //leave distortion on as default
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

  tft.println("april fools");
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
      if (bypassOn == false) //off and turning it on
      {
        tft.println("bypass on");
        flangeOutCord.disconnect();
        fvOutCord.disconnect();
        distOutCord.disconnect();
        bypassCord.connect();
        delay(300);
        bypassOn = true;
      }
      else
      {
        tft.println("bypass off");
        bypassCord.disconnect();
        bypassOn = false;
        if (fvOn == true)
        {
          fvOutCord.connect();
        }
        if (distOn == true)
        {
          distOutCord.connect();
        }
        if (flangeOn == true)
        {
          flangeOutCord.connect();
        }
        delay(300);
      }
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
        Connections();
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
      else if (inChar == '4')
      {
        saveState();
      }
      else if (inChar == '5')
      {
        saveLoader();
      }
   }
 }
}

void Connections()
{
    tft.println("Enter Effect to toggle"); //the logic behind the loop is that the user can enter up to 10 effects MAX.
    // when the user sends a 'z' it is like they are hitting 'done' in the app
    while (Serial1.available()==0) {}
    currentEffect = Serial1.read();
    
    switch(currentEffect)
    {
      case '1': //let 1 be flange
        if (flangeOn == false)//off turning on
        {
          tft.println("Flange on");
          flangeOutCord.connect();
          flangeOn = true;
        }
        else //on turning off
        {
          tft.println("Flange off");
          flangeOutCord.disconnect();
          flangeOn = false;
        }
        break;
      case '2': //let 2 be amp
        if (distOn == false) //off  turning on
        {
          tft.println("distortion on");
          distOutCord.connect();
          distOn = true;
        }
        else //on  turning off
        {
          tft.println("distortion off");
          distOutCord.disconnect();
          distOn = false;
        }
        break;
      case '3': //let 3 be freeverb
        //code here
        if (fvOn == false) //off  turning on
        {
          tft.println("reverb on");
          fvOutCord.connect();
          fvOn = true;
        }
        else //on  turning off
        {
          tft.println("reverb off");
          fvOutCord.disconnect();
          fvOn = false;
        }
        break;
      case 'z': //terminating case
          tft.println("Creation cancelled");
          break;
    }
  tft.println("done with toggling");
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
    while (Serial1.available()==0) {}
    param = Serial1.read();
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

void saveState()
{
  tft.println("Which save state");
  while (Serial1.available()==0) {}
  char rc = Serial1.read(); //determines which save state
  if (rc == '1')
  {
    save1[0] = flangeOn;
    save1[1] = distOn;
    save1[2] = fvOn;
    
    paramsSave1[0] = flangeOffset;
    paramsSave1[1] = flangeDepth;
    paramsSave1[2] = flangeModFreq; 
    paramsSave1[3] = ampGain;
    paramsSave1[4] = fvDry; 
    paramsSave1[5] = fvWet;
    paramsSave1[6] = fvRoomSize;
    paramsSave1[7] = fvDamp;
    tft.println("saved to 1");
  }
  else if (rc == '2')
  {
    save2[0] = flangeOn;
    save2[1] = distOn;
    save2[2] = fvOn;
    
    paramsSave2[0] = flangeOffset;
    paramsSave2[1] = flangeDepth;
    paramsSave2[2] = flangeModFreq; 
    paramsSave2[3] = ampGain;
    paramsSave2[4] = fvDry; 
    paramsSave2[5] = fvWet;
    paramsSave2[6] = fvRoomSize;
    paramsSave2[7] = fvDamp;
    tft.println("saved to 2");
  }
  else if (rc == '3')
  {
    save3[0] = flangeOn;
    save3[1] = distOn;
    save3[2] = fvOn;
    
    paramsSave3[0] = flangeOffset;
    paramsSave3[1] = flangeDepth;
    paramsSave3[2] = flangeModFreq; 
    paramsSave3[3] = ampGain;
    paramsSave3[4] = fvDry; 
    paramsSave3[5] = fvWet;
    paramsSave3[6] = fvRoomSize;
    paramsSave3[7] = fvDamp;
    tft.println("saved to 3");
  }
}

void saveLoader()
{
  //these few commands are to disconnect all effects and reset their status
  flangeOutCord.disconnect();
  fvOutCord.disconnect();
  distOutCord.disconnect();
  bypassCord.disconnect();
  
  tft.println("load which save?");
  while (Serial1.available()==0) {}
  char rc = Serial1.read();
  if (rc == '1')
  {
    if (save1[0] == true)
    {
      flangeOutCord.connect();
    }
    if (save1[1] == true)
    {
      distOutCord.connect();
    }
    if (save1[2] == true)
    {
      fvOutCord.connect();
    }
    flangeBlock.voices(paramsSave1[0], paramsSave1[1], paramsSave1[2]);
    ampBlock.gain(paramsSave1[3]);
    mixFVOut.gain(1, paramsSave1[4]);
    mixFVOut.gain(0, paramsSave1[5]);
    freeverbBlock.roomsize(paramsSave1[6]);
    freeverbBlock.damping(paramsSave1[7]);
    tft.println("loaded 1");
  }
  else if (rc == '2')
  {
    if (save2[0] == true)
    {
      flangeOutCord.connect();
    }
    if (save2[1] == true)
    {
      distOutCord.connect();
    }
    if (save2[2] == true)
    {
      fvOutCord.connect();
    }
    flangeBlock.voices(paramsSave2[0], paramsSave2[1], paramsSave2[2]);
    ampBlock.gain(paramsSave2[3]);
    mixFVOut.gain(1, paramsSave2[4]);
    mixFVOut.gain(0, paramsSave2[5]);
    freeverbBlock.roomsize(paramsSave2[6]);
    freeverbBlock.damping(paramsSave2[7]);
    tft.println("loaded 2");
  }
  else if (rc == '3')
  {
    if (save3[0] == true)
    {
      flangeOutCord.connect();
    }
    if (save3[1] == true)
    {
      distOutCord.connect();
    }
    if (save3[2] == true)
    {
      fvOutCord.connect();
    }
    flangeBlock.voices(paramsSave3[0], paramsSave3[1], paramsSave3[2]);
    ampBlock.gain(paramsSave3[3]);
    mixFVOut.gain(1, paramsSave3[4]);
    mixFVOut.gain(0, paramsSave3[5]);
    freeverbBlock.roomsize(paramsSave3[6]);
    freeverbBlock.damping(paramsSave3[7]);
    tft.println("loaded 3");
  }
}
