#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3

#define TFT_DC      9
#define TFT_CS      37
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

void setup() {
 // If you need a delay for power or some other thing
 delay(10);

 // Setup Serial ports
 Serial1.begin(9600); //rx1 and tx1 = pins 0 and 1 on Teensy for BT
 //Serial.begin(9600); // Default Serial over USB

  Serial.begin(9600);
  delay(500);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setFont(Arial_16);
  //tft.setTextSize(3);
  //tft.setCursor(40, 8);
  //tft.println("Peak Meter");

 delay(200);

} // end setup

void loop() 
{
 char inChar;
 //if(Serial.available() > 0) // use for USB
 if(Serial1.available() > 0) // use for BT
 {
    tft.println("Serial Available");
    inChar = (char)Serial1.read();

   if(Serial1.availableForWrite())
   {
     Serial1.println(inChar); // BT
     tft.println(inChar);
   }
 }




} // end loop
