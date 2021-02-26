#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3

#define TFT_DC      9
#define TFT_CS      37
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

// GUItool: begin automatically generated code
AudioControlSGTL5000     sgtl5000_1;     //xy=264.0056838989258,526.0000190734863
AudioInputI2S            lineIn;         //xy=55,303.9999704360962
AudioEffectFreeverb      freeverbBlock;      //xy=378.005615234375,364.914758682251
AudioEffectFlange        flangeBlock;        //xy=493.0056381225586,249.91472816467285
AudioAmplifier           ampBlock;           //xy=505.0056610107422,178.00565910339355
AudioMixer4              mixFVOut;         //xy=588.0055847167969,342.0055179595947
AudioOutputI2S           lineOut;        //xy=1031.0056114196777,313.99991607666016
AudioConnection          fvMixCord(freeverbBlock, 0, mixFVOut, 1);
AudioConnection*         Cord1;
AudioConnection*         Cord2;
AudioConnection*         Cord3;
AudioConnection*         Cord4;
AudioConnection*         Cord5;
AudioConnection*         Cord6;
AudioConnection*         Cord7;
AudioConnection*         lastCord;
AudioConnection*         fvDryCord;
//AudioConnection *cables[16];
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
  
  // setup SGTL5000
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.volume(0.8);
  sgtl5000_1.muteHeadphone();
  sgtl5000_1.unmuteLineout();

  // flange setup
  short delayline[5*AUDIO_BLOCK_SAMPLES];   
  flangeBlock.begin(delayline, 5*AUDIO_BLOCK_SAMPLES, (5*AUDIO_BLOCK_SAMPLES)/4, (5*AUDIO_BLOCK_SAMPLES)/4, 1);
  flangeBlock.voices((5*AUDIO_BLOCK_SAMPLES)/4, (5*AUDIO_BLOCK_SAMPLES)/4, 1);

  // Amp setup
  ampBlock.gain(15);

  // freeverb setup
  mixFVOut.gain(0, 0.9); // hear 90% "wet"
  mixFVOut.gain(1, 0.4); // and  10% "dry"
  freeverbBlock.roomsize(0.6);
  freeverbBlock.damping(0.4);

  tft.println("We're getting there!");
}

int activeEffect;
const byte numChars = 10;
char EffArr[numChars];
int numEff;

void loop() 
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
        Orderer();
      }
      else if (inChar == '2')
      {
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(0, 8);
      }
   }
 }
}

void Orderer() //int numEff
{
    // loop variable initialization
    static byte ndx = 0;
    char endMarker = 'z';
    char rc;
    int cnt = 0;
    numEff = 0;

    tft.println("Enter Effect Names"); //the logic behind the loop is that the user can enter up to 10 effects MAX.
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
    tft.println("Chain Created");
    Connections();
}

void Connections()
{
  // delete wire connections to avoid messing things up
  delete Cord1;
  delete Cord2;
  delete Cord3;
  delete Cord4;
  delete Cord5;
  delete Cord6;
  delete Cord7;
  delete lastCord;
  delete fvDryCord;
  
  bool breaker = false;
  tft.println("in connections");
  for (int i = 0; i <= 9; i++)
  {
//    tft.print("element is ");
//    tft.println(EffArr[i]);
//    char y = EffArr[i];
    
    switch(EffArr[i])
    {
      case '1': //let 1 be flange
        if (i == 0)
        {
          Cord1 = new AudioConnection(lineIn, 1, flangeBlock, 0);
          tft.println("1st = flange");
        }
        else
        {
          switch(EffArr[i-1]) //figuring out what the previous effect was if the effect isn't the 1st
          {
            case '2': // 2 is amp
              Cord2 = new AudioConnection(ampBlock, 0, flangeBlock, 0);
              tft.print(i);
              tft.println(" effect is flange");
              break;
            case '3': // 3 is freeverb
              //fvDryCord = new AudioConnection(flangeBlock, 0, freeverbBlock, 0);
              Cord3 = new AudioConnection(mixFVOut, 0, flangeBlock, 0);
              tft.print(i);
              tft.println(" effect is flange");
              break;
          }   
        }
        break;
      case '2': //let 2 be amp
        if (i == 0)
        {
          Cord1 = new AudioConnection(lineIn, 1, ampBlock, 0);
          tft.println("1st = amp");
        }
        else
        {
          switch(EffArr[i-1])
          {
            case '1': // 1 is flange
              Cord4 = new AudioConnection(flangeBlock, 0, ampBlock, 0);
              tft.print(i);
              tft.println(" effect is amp");
              break;
            case '3': // 3 is freeverb
              Cord5 = new AudioConnection(mixFVOut, 0, ampBlock, 0);
              //fvDryCord = new AudioConnection(ampBlock, 0, mixFVOut, 0);
              tft.print(i);
              tft.println(" effect is amp");
              break;
          }   
        }
        break;
      case '3': //let 3 be freeverb
        if (i == 0)
        {
          Cord1 = new AudioConnection(lineIn, 1, freeverbBlock, 0); //the cord that connects FV to the mix is hard coded
          fvDryCord = new AudioConnection(lineIn, 1, mixFVOut, 0);
          tft.println("1st = freeverb");
        }
        else
        {
          switch(EffArr[i-1])
          {
            case '1': // 1 is flange
              Cord6 = new AudioConnection(flangeBlock, 0, mixFVOut, 0);
              fvDryCord = new AudioConnection(flangeBlock, 0, freeverbBlock, 0);
              tft.print(i);
              tft.println(" effect is freeverb");
              break;
            case '2': // 2 is amp
              Cord7 = new AudioConnection(ampBlock, 0, freeverbBlock, 0);
              fvDryCord = new AudioConnection(ampBlock, 0, mixFVOut, 0);
              tft.print(i);
              tft.println(" effect is freeverb");
              break;
          }   
        }
        break;
        
      case 'z': //terminating case
        switch(EffArr[i-1])
        {
          case '1': //flange
            lastCord = new AudioConnection(flangeBlock, 0, lineOut, 1);
            tft.println("last connection made");
            break;
          case '2': //amp
            lastCord = new AudioConnection(ampBlock, 0, lineOut, 1);
            tft.println("last connection made");
            break;
          case '3': //freeverb
            lastCord = new AudioConnection(mixFVOut, 0, lineOut, 1);
            tft.println("last connection made");
            break;
        }
        breaker = true;
        break;
    }
    if (breaker == true) //if the terminating character has been reached, just break out of the loop
    {
      break;
    }
  }
  tft.println("done with connections");
}
