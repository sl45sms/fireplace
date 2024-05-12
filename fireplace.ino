/**************************************************************************
 * 
 * ESP-12-E module with ST7789 display that shows open flames
 * surrounded by bricks to simulate a fireplace
 * This is a free software with NO WARRANTY.
 * The code is an adaptation from examples of the excellent AnimatedGIF lib
 *
 *************************************************************************/
#include <AnimatedGIF.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include "firegiphy.h"

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

// ST7789 TFT module connections
#define TFT_DC    D1     // TFT DC  pin is connected to NodeMCU pin D1 (GPIO5)   esp12-f/e pin 20
#define TFT_RST   D2     // TFT RST pin is connected to NodeMCU pin D2 (GPIO4)   esp12-f/e pin 19
#define TFT_CS    D8     // TFT CS  pin is connected to NodeMCU pin D8 (GPIO15)  esp12-f/e pin 16 - may not used on all displays
#define TFT_SDA   D7     // TFT SDA pin is connected to NodeMCU pin D7 (GPIO13)  esp12-f/e pin 7
#define TFT_SCL   D5     // TFT SCL pin is connected to NodeMCU pin D5 (GPIO14)  esp12-f/e pin 5

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
AnimatedGIF gif;

// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[512];
    int x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth + pDraw->iX > DISPLAY_WIDTH)
       iWidth = DISPLAY_WIDTH - pDraw->iX;
    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line
    if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
       return; 
    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
      for (x=0; x<iWidth; x++)
      {
        if (s[x] == pDraw->ucTransparent)
           s[x] = pDraw->ucBackground;
      }
      pDraw->ucHasTransparency = 0;
    }

    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      int x, iCount;
      pEnd = s + iWidth;
      x = 0;
      iCount = 0; // count non-transparent pixels
      while(x < iWidth)
      {
        c = ucTransparent-1;
        d = usTemp;
        while (c != ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent) // done, stop
          {
            s--; // back up to treat it like transparent
          }
          else // opaque
          {
             *d++ = usPalette[c];
             iCount++;
          }
        } // while looking for opaque pixels
        if (iCount) // any opaque pixels?
        {
          tft.startWrite();
          tft.setAddrWindow(pDraw->iX+x, y, iCount, 1);
          tft.writePixels(usTemp, iCount, false, false);
          tft.endWrite();
          x += iCount;
          iCount = 0;
        }
        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent)
             iCount++;
          else
             s--; 
        }
        if (iCount)
        {
          x += iCount; // skip these
          iCount = 0;
        }
      }
    }
    else
    {
      s = pDraw->pPixels;
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      for (x=0; x<iWidth; x++)
        usTemp[x] = usPalette[*s++];
      tft.startWrite();
      tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
      tft.writePixels(usTemp, iWidth, false, false);
      tft.endWrite();
    }
} /* GIFDraw() */

void welcomedrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void setup(void) {
  Serial.begin(115200);
  Serial.print(F("Hello! ST7789 TFT Fireplace "));

  // if the display has CS pin try with SPI_MODE0
  tft.init(DISPLAY_WIDTH, DISPLAY_HEIGHT, SPI_MODE2);    // Init ST7789 display 240x240 pixel
  // if the screen is flipped, remove this command
  tft.setRotation(0);
  // large block of text
  tft.fillScreen(ST77XX_BLACK);
  welcomedrawtext("Let's burn some woods", ST77XX_WHITE);
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);
  gif.begin(GIF_PALETTE_RGB565_LE);

}

void loop() {
  if (gif.openFLASH((uint8_t *)ucFire, sizeof(ucFire), GIFDraw)) //sould be openFLASH for esp8266, if you read from PROGMEM
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    while (gif.playFrame(true, NULL)){
      yield(); //to fix "Soft WDT reset" On ESP8266 watchdog (WDT) turned on by default.
    }
    gif.close();
  }
}
