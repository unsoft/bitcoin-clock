#include "displayDriver.h"

#ifdef V1_DISPLAY

#include <TFT_eSPI.h>
#include "media/myFonts.h"
#include "media/Free_Fonts.h"
#include "version.h"
#include "monitor.h"
#include "OpenFontRender.h"
#include "rotation.h"

#define WIDTH 240
#define HEIGHT 135

OpenFontRender render;
TFT_eSPI tft = TFT_eSPI();                  // Invoke library, pins defined in User_Setup.h
TFT_eSprite background = TFT_eSprite(&tft); // Invoke library sprite

void tDisplay_Init(void)
{
  tft.init();
  tft.setRotation(ROTATION_90);
  tft.setSwapBytes(true);                 // Swap the colour byte order when rendering
  background.createSprite(WIDTH, HEIGHT); // Background Sprite
  background.setSwapBytes(true);
  render.setDrawer(background);  // Link drawing object to background instance (so font will be rendered on background)
  render.setLineSpaceRatio(0.9); // Espaciado entre texto

  // Load the font and check it can be read OK
  // if (render.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) {
  if (render.loadFont(DigitalNumbers, sizeof(DigitalNumbers)))
  {
    Serial.println("Initialise error");
    return;
  }
}

void tDisplay_AlternateScreenState(void)
{
  int screen_state = digitalRead(TFT_BL);
  Serial.println("Switching display state");
  digitalWrite(TFT_BL, !screen_state);
}

void tDisplay_AlternateRotation(void)
{
  tft.setRotation( flipRotation(tft.getRotation()) );
}

void drawClockPage(const clock_data& data, bool showPrice)
{
  const uint16_t gold = 0xDEB2;
  const uint16_t darkGold = 0x7B6A;
  const int tileY = 29;
  const int tileHeight = 70;
  String digits = showPrice ? data.btcPrice : data.blockHeight;

  if (showPrice && digits.startsWith("$")) digits.remove(0, 1);
  if (digits.isEmpty()) digits = "0";
  if (!showPrice && digits.length() > 7) digits = digits.substring(digits.length() - 7);
  if (showPrice && digits.length() > 6) digits = digits.substring(digits.length() - 6);

  const int tileCount = digits.length() + (showPrice ? 1 : 0);
  const int tileWidth = tileCount == 7 ? 29 : 31;
  const int gap = 3;
  const int rowWidth = tileCount * tileWidth + (tileCount - 1) * gap;
  const int startX = (WIDTH - rowWidth) / 2;

  background.fillSprite(TFT_BLACK);
  background.setTextFont(FONT2);
  background.setTextSize(1);
  background.setTextDatum(TC_DATUM);
  background.setTextColor(gold, TFT_BLACK);
  background.drawString(showPrice ? "BTC / USDT" : "CURRENT BLOCK", WIDTH / 2, 9, FONT2);

  render.setFontSize(47);
  for (int index = 0; index < tileCount; index++)
  {
    const int tileX = startX + index * (tileWidth + gap);
    background.drawRect(tileX, tileY, tileWidth, tileHeight, gold);
    background.drawRect(tileX + 2, tileY + 2, tileWidth - 4, tileHeight - 4, darkGold);

    if (showPrice && index == 0)
    {
      background.setTextDatum(MC_DATUM);
      background.setTextColor(TFT_WHITE, TFT_BLACK);
      background.drawString("$", tileX + tileWidth / 2, tileY + tileHeight / 2, FONT4);
    }
    else
    {
      const int digitIndex = index - (showPrice ? 1 : 0);
      String digit = digits.substring(digitIndex, digitIndex + 1);
      render.cdrawString(digit.c_str(), tileX + tileWidth / 2, tileY + 10, TFT_WHITE);
    }
  }

  background.setTextFont(FONT2);
  background.setTextSize(2);
  background.setTextDatum(MC_DATUM);
  background.setTextColor(gold, TFT_BLACK);
  background.drawString("BITCOIN CLOCK", WIDTH / 2, 119, FONT2);
  background.pushSprite(0, 0);
}

void tDisplay_BlockHeightScreen(unsigned long mElapsed)
{
  drawClockPage(getClockData(mElapsed), false);
}

void tDisplay_BTCPriceScreen(unsigned long mElapsed)
{
  drawClockPage(getClockData(mElapsed), true);
}

void tDisplay_LoadingScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.drawFastHLine(24, 43, 192, 0x7B6A);
  tft.drawFastHLine(24, 91, 192, 0x7B6A);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0xDEB2, TFT_BLACK);
  tft.setTextFont(FONT4);
  tft.drawString("BITCOIN CLOCK", WIDTH / 2, 67, FONT4);
  tft.setTextFont(FONT2);
  tft.setTextColor(0x7B6A, TFT_BLACK);
  tft.drawString(CURRENT_VERSION, WIDTH / 2, 111, FONT2);
}

void tDisplay_SetupScreen(void)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0xDEB2, TFT_BLACK);
  tft.setTextFont(FONT4);
  tft.drawString("BITCOIN CLOCK", WIDTH / 2, 44, FONT4);
  tft.drawFastHLine(24, 67, 192, 0x7B6A);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(FONT2);
  tft.drawString("CONNECT TO WIFI", WIDTH / 2, 88, FONT2);
  tft.drawString("BitcoinClock", WIDTH / 2, 108, FONT2);
}

void tDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

void tDisplay_DoLedStuff(unsigned long frame)
{
}

CyclicScreenFunction tDisplayCyclicScreens[] = {tDisplay_BlockHeightScreen, tDisplay_BTCPriceScreen};

DisplayDriver tDisplayV1Driver = {
    tDisplay_Init,
    tDisplay_AlternateScreenState,
    tDisplay_AlternateRotation,
    tDisplay_LoadingScreen,
    tDisplay_SetupScreen,
    tDisplayCyclicScreens,
    tDisplay_AnimateCurrentScreen,
    tDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(tDisplayCyclicScreens),
    0,
    WIDTH,
    HEIGHT};
#endif
