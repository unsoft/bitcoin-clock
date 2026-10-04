#include "displayDriver.h"

#ifdef LILYGO_S3_T_DISPLAY

#include <TFT_eSPI.h>
#include "monitor.h"

void setBitcoinClockBacklightLevel(uint8_t level);

namespace
{
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 170;
constexpr uint16_t GOLD = 0xDEB2;
constexpr uint16_t DIM_GOLD = 0x7B6A;
constexpr uint16_t WHITE = TFT_WHITE;
constexpr uint16_t BLACK = TFT_BLACK;
constexpr int TILE_Y = 12;
constexpr int TILE_HEIGHT = 92;
constexpr int TILE_GAP = 3;
constexpr int SIDE_MARGIN = 8;
constexpr int VALUE_TILE_COUNT = 7;
constexpr uint8_t BACKLIGHT_PWM_CHANNEL = 7;
constexpr uint8_t BACKLIGHT_LEVELS[] = {0, 64, 128, 192, 255};
constexpr uint8_t BACKLIGHT_LEVEL_COUNT = sizeof(BACKLIGHT_LEVELS) / sizeof(BACKLIGHT_LEVELS[0]);

TFT_eSPI tft;
uint8_t backlightLevel = BACKLIGHT_LEVEL_COUNT - 1;
int renderedPage = -1;
String renderedValue;
bool renderedLabelVisible = false;
bool renderedCurrencyVisible = false;

void drawTile(int x, int width)
{
  tft.drawRoundRect(x, TILE_Y, width, TILE_HEIGHT, 4, GOLD);
  tft.drawRoundRect(x + 2, TILE_Y + 2, width - 4, TILE_HEIGHT - 4, 3, DIM_GOLD);
}

void drawLabel(int x, int width, const char* line1, const char* line2)
{
  tft.setTextColor(GOLD, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(1);
  tft.drawString(line1, x + width / 2, TILE_Y + 35);
  tft.drawString(line2, x + width / 2, TILE_Y + 53);
}

void drawDigit(int x, int width, char digit)
{
  tft.fillRect(x + 4, TILE_Y + 4, width - 8, TILE_HEIGHT - 8, BLACK);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(7);
  tft.drawString(String(digit), x + width / 2, TILE_Y + TILE_HEIGHT / 2 + 1);
}

void drawFooter(uint8_t page)
{
  const int centerX = SCREEN_WIDTH / 2;
  tft.drawFastHLine(centerX - 65, 137, 46, DIM_GOLD);
  tft.drawFastHLine(centerX + 19, 137, 46, DIM_GOLD);
  tft.setTextColor(GOLD, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.drawString("BITCOIN CLOCK", centerX, 120);
  tft.setTextFont(1);
  tft.drawString("mini", centerX, 143);

  tft.setTextColor(DIM_GOLD, BLACK);
  tft.setTextFont(1);
  tft.drawString(String(page + 1) + " / 3", SCREEN_WIDTH - 20, SCREEN_HEIGHT - 10);
}

void drawFrame()
{
  tft.drawRoundRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, 8, DIM_GOLD);
  tft.fillCircle(11, 11, 2, DIM_GOLD);
  tft.fillCircle(SCREEN_WIDTH - 11, 11, 2, DIM_GOLD);
  tft.fillCircle(11, SCREEN_HEIGHT - 11, 2, DIM_GOLD);
  tft.fillCircle(SCREEN_WIDTH - 11, SCREEN_HEIGHT - 11, 2, DIM_GOLD);
}

void drawPage(const char* line1, const char* line2, const String& value, bool price, bool time)
{
  String digits = value;
  if (price)
  {
    digits.replace(",", "");
    digits.replace(".", "");
  }
  if (digits.length() == 0)
    digits = "--";

  const int page = currentDisplayDriver->current_cyclic_screen;
  const int prefixTiles = price ? 2 : 1;
  const bool showLabel = time || digits.length() <= VALUE_TILE_COUNT - prefixTiles;
  const bool showCurrency = price && digits.length() <= VALUE_TILE_COUNT - 1;
  const int tileCount = time ? 1 + digits.length() + 1 : VALUE_TILE_COUNT;
  const int digitCapacity = time
                                ? digits.length()
                                : VALUE_TILE_COUNT - (showLabel ? 1 : 0) - (showCurrency ? 1 : 0);
  if (digits.length() > digitCapacity)
    digits = "-------";

  const int availableWidth = SCREEN_WIDTH - SIDE_MARGIN * 2;
  const int tileWidth = (availableWidth - (tileCount - 1) * TILE_GAP) / tileCount;
  const int rowWidth = tileCount * tileWidth + (tileCount - 1) * TILE_GAP;
  const int startX = (SCREEN_WIDTH - rowWidth) / 2;
  const bool redrawLayout = page != renderedPage ||
                            showLabel != renderedLabelVisible ||
                            showCurrency != renderedCurrencyVisible;

  if (redrawLayout)
  {
    tft.fillScreen(BLACK);
    drawFrame();
  }

  int x = startX;
  if (showLabel)
  {
    if (redrawLayout)
    {
      drawTile(x, tileWidth);
      drawLabel(x, tileWidth, line1, line2);
    }
    x += tileWidth + TILE_GAP;
  }

  if (showCurrency)
  {
    if (redrawLayout)
    {
      drawTile(x, tileWidth);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextFont(4);
      tft.drawString("$", x + tileWidth / 2, TILE_Y + TILE_HEIGHT / 2);
    }
    x += tileWidth + TILE_GAP;
  }

  for (int i = 0; i < digitCapacity; ++i)
  {
    if (redrawLayout)
      drawTile(x, tileWidth);

    const char digit = i < digits.length() ? digits[i] : ' ';
    if (redrawLayout || i >= renderedValue.length() || renderedValue[i] != digit)
      drawDigit(x, tileWidth, digit);

    x += tileWidth + TILE_GAP;

    if (time && i == 1 && i + 1 < digits.length())
    {
      if (redrawLayout)
      {
        drawTile(x, tileWidth);
        tft.setTextColor(WHITE, BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.drawString(":", x + tileWidth / 2, TILE_Y + TILE_HEIGHT / 2);
      }
      x += tileWidth + TILE_GAP;
    }
  }

  if (redrawLayout)
    drawFooter(page);

  renderedPage = page;
  renderedValue = digits;
  renderedLabelVisible = showLabel;
  renderedCurrencyVisible = showCurrency;
}

void drawBlockPage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  drawPage("CURRENT", "BLOCK", data.blockHeight, false, false);
}

void drawPricePage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  drawPage("BTC", "USDT", data.btcPrice, true, false);
}

void drawTimePage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  String time = data.currentTime;
  time.replace(":", "");
  drawPage("LOCAL", "TIME", time, false, true);
}

void initDisplay()
{
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  ledcSetup(BACKLIGHT_PWM_CHANNEL, 5000, 8);
  ledcAttachPin(TFT_BL, BACKLIGHT_PWM_CHANNEL);
  ledcWrite(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_LEVELS[backlightLevel]);
  tft.fillScreen(BLACK);
  drawFrame();
}

void cycleBacklightBrightness()
{
  setBitcoinClockBacklightLevel((backlightLevel + 1) % BACKLIGHT_LEVEL_COUNT);
}

void loadingScreen()
{
  tft.fillScreen(BLACK);
  drawFrame();
  tft.setTextColor(GOLD, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.drawString("BITCOIN CLOCK", SCREEN_WIDTH / 2, 78);
}

void setupScreen()
{
  tft.fillScreen(BLACK);
  drawFrame();
  tft.setTextColor(GOLD, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.drawString("BITCOIN CLOCK", SCREEN_WIDTH / 2, 58);
  tft.setTextFont(2);
  tft.setTextColor(WHITE, BLACK);
  tft.drawString("CONNECT TO WIFI", SCREEN_WIDTH / 2, 94);
  tft.drawString("BitcoinClock", SCREEN_WIDTH / 2, 116);
}

void drawWifiFailureScreen()
{
  tft.fillScreen(BLACK);
  drawFrame();
  tft.setTextColor(GOLD, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.drawString("BITCOIN CLOCK", SCREEN_WIDTH / 2, 52);
  tft.setTextColor(WHITE, BLACK);
  tft.setTextFont(2);
  tft.drawString("WIFI CONNECTION FAILED", SCREEN_WIDTH / 2, 84);
  tft.drawString("CHECK SAVED WIFI SETTINGS", SCREEN_WIDTH / 2, 106);
  tft.drawString("CONNECT TO BitcoinClock", SCREEN_WIDTH / 2, 128);
}

void noRotationChange() {}
void noAnimation(unsigned long) {}
void noLedAction(unsigned long) {}

CyclicScreenFunction pages[] = {drawBlockPage, drawPricePage, drawTimePage};
}

void drawBitcoinClockWifiFailureScreen()
{
  drawWifiFailureScreen();
}

void setBitcoinClockBacklightLevel(uint8_t level)
{
  if (level >= BACKLIGHT_LEVEL_COUNT)
    return;

  backlightLevel = level;
  ledcWrite(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_LEVELS[backlightLevel]);
  Serial.printf("[DISPLAY] Backlight brightness: %u%%\n",
                static_cast<unsigned int>(BACKLIGHT_LEVELS[backlightLevel]) * 100 / 255);
}

DisplayDriver bitcoinClockS3DisplayDriver = {
    initDisplay,
    cycleBacklightBrightness,
    noRotationChange,
    loadingScreen,
    setupScreen,
    pages,
    noAnimation,
    noLedAction,
    SCREENS_ARRAY_SIZE(pages),
    0,
    SCREEN_WIDTH,
    SCREEN_HEIGHT};

#endif
