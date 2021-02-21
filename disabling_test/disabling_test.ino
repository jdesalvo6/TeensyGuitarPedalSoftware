#include <Bounce.h>
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
AudioInputI2S            lineIn;           //xy=154.00569915771484,454.82383728027344
AudioEffectFlange        flangeBlock;        //xy=332.0056686401367,450.82378005981445
AudioMixer4              DelayIn;         //xy=538.0055313110352,282.82368087768555
AudioEffectDelay         delayBlock;         //xy=558.0054550170898,420.82371711730957
AudioMixer4              DelayOut;         //xy=762.0053253173828,549.823657989502
AudioAmplifier           amp1;           
AudioOutputI2S           lineOut;           //xy=1007.0056381225586,548.8237571716309
AudioConnection*          patchCord1; //(lineIn, 1, flangeBlock, 0);
AudioConnection*          patchCord2; //(flangeBlock, 0, DelayIn, 0);
AudioConnection*          patchCord3; //(flangeBlock, 0, DelayOut, 1);
AudioConnection           patchCord4(DelayIn, delayBlock);
AudioConnection           patchCord5(delayBlock, 0, DelayIn, 1);
AudioConnection           patchCord6(delayBlock, 0, DelayOut, 0);
AudioConnection*          patchCord7; //(DelayOut, 0, lineOut, 1);
AudioConnection           patchCord8(amp1, 0, lineOut, 1);
AudioConnection*          patchCord9; //(DelayOut, 0, lineOut, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240.00570678710938,798.8238391876221
// GUItool: end automatically generated code

const int buttonPinFlange = 28;
const int buttonPinDelay = 29;
Bounce pushbuttonFlange = Bounce(buttonPinFlange, 1);
Bounce pushbuttonDelay = Bounce(buttonPinDelay, 1);

void setup() {
  // put your setup code here, to run once:
   Serial1.begin(9600); //rx1 and tx1 = pins 0 and 1 on Teensy for BT
  Serial.begin(9600);
  delay(500);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setFont(Arial_16);
  //tft.setTextSize(3);


  
  delay(250);
  AudioMemory(524);
  //Serial.begin(9600);
  pinMode(buttonPinFlange, INPUT_PULLUP);
  pinMode(buttonPinDelay, INPUT_PULLUP);

  //setup SGTL5000
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  //sgtl5000_1.volume(0.3);
  sgtl5000_1.muteHeadphone();
  sgtl5000_1.unmuteLineout();

  short delayline[5*AUDIO_BLOCK_SAMPLES];   
  flangeBlock.begin(delayline, 5*AUDIO_BLOCK_SAMPLES, (5*AUDIO_BLOCK_SAMPLES)/4, (5*AUDIO_BLOCK_SAMPLES)/4, 1);
  flangeBlock.voices((5*AUDIO_BLOCK_SAMPLES)/4, (5*AUDIO_BLOCK_SAMPLES)/4, 1);

  DelayIn.gain(0, 0.5);
  DelayIn.gain(1, 0.7);

  delayBlock.delay(0, 300);

  amp1.gain(50);

  tft.println("Order Switching Test");
}

unsigned int mode = 0;
int flange = 1;
int delayy = 1;
int amp = 1;

void loop()
{

 char inChar;
 if(Serial1.available() > 0) // use for BT
 {
    inChar = (char)Serial1.read();

   if(Serial1.availableForWrite())
   {
      if (inChar == '1' && mode != 1 ) //mode 1 is for flange first delay second
      {
        mode = 1;
        delete patchCord1;
        delete patchCord2;
        delete patchCord3;
        delete patchCord7;
        
        patchCord1 = new AudioConnection(lineIn, 1, flangeBlock, 0);
        patchCord2 = new AudioConnection(flangeBlock, 0, DelayIn, 0);
        patchCord3 = new AudioConnection(flangeBlock, 0, DelayOut, 1);
        patchCord7 = new AudioConnection(DelayOut, 0, amp1, 0);

        Serial1.println("Order Set: Flange -> Delay");
        tft.println("Order Set: Flange -> Delay"); // BT
      }
      else if (inChar == '1' && mode == 1)
      {
        Serial1.println("Mode already engaged");
        tft.println("Mode already engaged"); // BT
      }
      if (inChar == '2' && mode != 2)
      {
        mode = 2;
        delete patchCord1;
        delete patchCord2;
        delete patchCord3;
        delete patchCord7;
        
        patchCord1 = new AudioConnection(lineIn, 1, DelayIn, 0);
        patchCord2 = new AudioConnection(lineIn, 1, DelayOut, 1);
        patchCord3 = new AudioConnection(DelayOut, 0, flangeBlock, 0);
        patchCord7 = new AudioConnection(flangeBlock, 0, amp1, 0);

        tft.println("Order Set: Delay -> Flange"); // BT
        Serial1.println("Order Set: Delay -> Flange");
      }
      else if (inChar == '2' && mode == 2)
      {
        tft.println("Mode already engaged"); // BT
        Serial1.println("Mode already engaged");
      }
      if (inChar == '3')
      {
        tft.println("Engaging or disengaging mode"); // BT
        Serial1.println("Engaging or disengaging mode");
        EffectSelect();
      }
   }
 }
}


void EffectSelect()
{
  tft.println("Which effect would you like off"); // For now just do the amp
  Serial1.println("Which effect would you like off"); //enter 3 for the last effect in the chain (amp)

  while(1)
  {
    char inChar = (char)Serial1.read();
    if (inChar == '3')
    {
      OnOff(inChar);
      break;
    }
  }
  Serial1.println("out of while loop");
}

void OnOff(char effect)
{
  tft.println("Would you like it on or off?"); // For now just do the amp
  Serial1.println("Would you like it on or off?");

  while(1)
  {
    char inChar = (char)Serial1.read();
    if (effect == '3' && inChar == '0')
    {
      patchCord8.disconnect();
      delete patchCord7;
      patchCord9 = new AudioConnection(flangeBlock, 0, lineOut, 1);
      break;
    }
    else if (effect == '3' && inChar == '1')
    {
      patchCord7 = new AudioConnection(flangeBlock, 0, amp1, 0);
      delete patchCord9;
      patchCord8.connect();
      break;
    } 
  }  
}
