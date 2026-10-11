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
constexpr uint8_t MANUAL_ON_BRIGHTNESS_LEVEL = 2;
constexpr uint8_t SCHEDULE_DISABLED_BRIGHTNESS_LEVEL = 3;

TFT_eSPI tft;
uint8_t backlightLevel = BACKLIGHT_LEVEL_COUNT - 1;
bool brightnessScheduleEnabled = true;
bool brightnessScheduleOffWindow = false;
bool manualBrightnessOverride = false;
int renderedPage = -1;
int renderedTileCount = -1;
int renderedDigitCapacity = -1;
String renderedValue;
String renderedFooterInfo;
String renderedFooterWonPrice;
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

void drawBoldLine(int x0, int y0, int x1, int y1, uint16_t color)
{
  for (int offset = -1; offset <= 1; ++offset)
    tft.drawLine(x0 + offset, y0, x1 + offset, y1, color);
}

String formatPriceWithCommas(const String& price)
{
  String formatted;
  const int firstGroupLength = price.length() % 3;
  for (int i = 0; i < price.length(); ++i)
  {
    if (i > 0 && (i - firstGroupLength) % 3 == 0)
      formatted += ',';
    formatted += price[i];
  }
  return formatted;
}

void drawFooter(uint8_t page, const String& info, const String& wonPrice)
{
  const int centerX = SCREEN_WIDTH / 2;
  tft.fillRect(10, 108, SCREEN_WIDTH - 20, 25, BLACK);
  tft.setTextColor(DIM_GOLD, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  if (wonPrice.isEmpty())
  {
    tft.drawString(info, centerX, 120);
  }
  else
  {
    const String formattedWonPrice = formatPriceWithCommas(wonPrice);
    const int infoWidth = tft.textWidth(info, 2);
    const int wonPriceWidth = tft.textWidth(formattedWonPrice, 2);
    constexpr int WON_SYMBOL_WIDTH = 14;
    constexpr int ITEM_GAP = 3;
    const int totalWidth = infoWidth + ITEM_GAP + WON_SYMBOL_WIDTH + ITEM_GAP + wonPriceWidth;
    const int startX = centerX - totalWidth / 2;
    const int symbolCenterX = startX + infoWidth + ITEM_GAP + WON_SYMBOL_WIDTH / 2;
    const int symbolCenterY = 120;

    tft.setTextDatum(TL_DATUM);
    tft.drawString(info, startX, 112);
    tft.drawLine(symbolCenterX - 5, symbolCenterY - 7,
                 symbolCenterX - 2, symbolCenterY + 7, DIM_GOLD);
    tft.drawLine(symbolCenterX - 2, symbolCenterY + 7,
                 symbolCenterX, symbolCenterY - 1, DIM_GOLD);
    tft.drawLine(symbolCenterX, symbolCenterY - 1,
                 symbolCenterX + 2, symbolCenterY + 7, DIM_GOLD);
    tft.drawLine(symbolCenterX + 2, symbolCenterY + 7,
                 symbolCenterX + 5, symbolCenterY - 7, DIM_GOLD);
    tft.drawFastHLine(symbolCenterX - 7, symbolCenterY - 3, 14, DIM_GOLD);
    tft.drawFastHLine(symbolCenterX - 7, symbolCenterY + 2, 14, DIM_GOLD);
    tft.drawString(formattedWonPrice,
                   symbolCenterX + WON_SYMBOL_WIDTH / 2 + ITEM_GAP,
                   112);
    tft.setTextDatum(MC_DATUM);
  }

  tft.drawFastHLine(16, 143, 62, DIM_GOLD);
  tft.drawFastHLine(SCREEN_WIDTH - 78, 143, 62, DIM_GOLD);
  tft.setTextColor(GOLD, BLACK);
  tft.setTextFont(2);
  tft.drawString("BITCOIN CLOCK", centerX, 143);

  tft.setTextColor(DIM_GOLD, BLACK);
  tft.setTextFont(1);
  tft.drawString(String(page + 1) + " / " +
                     String(currentDisplayDriver->num_cyclic_screens),
                 SCREEN_WIDTH - 20,
                 SCREEN_HEIGHT - 10);
}

void drawFrame()
{
  tft.drawRoundRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, 8, DIM_GOLD);
  tft.fillCircle(11, 11, 2, DIM_GOLD);
  tft.fillCircle(SCREEN_WIDTH - 11, 11, 2, DIM_GOLD);
  tft.fillCircle(11, SCREEN_HEIGHT - 11, 2, DIM_GOLD);
  tft.fillCircle(SCREEN_WIDTH - 11, SCREEN_HEIGHT - 11, 2, DIM_GOLD);
}

void drawPage(const char* line1,
              const char* line2,
              const String& value,
              int page,
              bool price,
              bool time,
              const String& footerInfo,
              const char* priceUnit = "",
              bool wonSign = false,
              const String& footerWonPrice = "")
{
  String digits = value;
  if (price)
  {
    digits.replace(",", "");
    digits.replace(".", "");
  }
  if (digits.length() == 0)
    digits = "--";

  if (currentDisplayDriver->current_cyclic_screen != page)
    return;

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
                            tileCount != renderedTileCount ||
                            digitCapacity != renderedDigitCapacity ||
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
      if (wonSign)
      {
        const int centerX = x + tileWidth / 2;
        const int centerY = TILE_Y + TILE_HEIGHT / 2;
        drawBoldLine(centerX - 12, centerY - 10, centerX - 7, centerY + 10, WHITE);
        drawBoldLine(centerX - 7, centerY + 10, centerX, centerY - 1, WHITE);
        drawBoldLine(centerX, centerY - 1, centerX + 7, centerY + 10, WHITE);
        drawBoldLine(centerX + 7, centerY + 10, centerX + 12, centerY - 10, WHITE);
        tft.fillRect(centerX - 14, centerY - 5, 28, 3, WHITE);
        tft.fillRect(centerX - 14, centerY + 3, 28, 3, WHITE);
      }
      else
      {
        tft.drawString(priceUnit, x + tileWidth / 2, TILE_Y + TILE_HEIGHT / 2);
      }
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

  if (redrawLayout ||
      footerInfo != renderedFooterInfo ||
      footerWonPrice != renderedFooterWonPrice)
    drawFooter(page, footerInfo, footerWonPrice);

  renderedPage = page;
  renderedTileCount = tileCount;
  renderedDigitCapacity = digitCapacity;
  renderedValue = digits;
  renderedFooterInfo = footerInfo;
  renderedFooterWonPrice = footerWonPrice;
  renderedLabelVisible = showLabel;
  renderedCurrencyVisible = showCurrency;
}

void drawBlockPage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  const String footerInfo =
      "$" + formatPriceWithCommas(data.btcPriceUsd) +
      " / BTC.D " + data.btcDominance + " / F&G " + data.fearGreedIndex;
  drawPage("CURRENT", "BLOCK", data.blockHeight, 2, false, false,
           footerInfo);
}

void drawDollarPricePage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  const String footerInfo =
      "BLOCK " + formatPriceWithCommas(data.blockHeight) +
      " / BTC.D " + data.btcDominance + " / F&G " + data.fearGreedIndex;
  drawPage("BTC", "USD", data.btcPriceUsd, 0, true, false, footerInfo, "$");
}

void drawWonPricePage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  const String footerInfo =
      "KRW-USD " + data.krwUsdRate + " / Premium " + data.kimchiPremium;
  drawPage("BTC", "KRW", data.btcPriceKrw, 1, true, false, footerInfo, "", true);
}

void drawTimePage(unsigned long)
{
  const bitcoin_clock_data data = getBitcoinClockData();
  String time = data.currentTime;
  time.replace(":", "");
  const String footerInfo =
      "$" + formatPriceWithCommas(data.btcPriceUsd) +
      " / BTC.D " + data.btcDominance + " / F&G " + data.fearGreedIndex;
  drawPage("LOCAL", "TIME", time, 3, false, true, footerInfo);
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
  if (brightnessScheduleEnabled && brightnessScheduleOffWindow && !manualBrightnessOverride)
  {
    manualBrightnessOverride = true;
    setBitcoinClockBacklightLevel(MANUAL_ON_BRIGHTNESS_LEVEL);
    return;
  }

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

CyclicScreenFunction pages[] = {
    drawDollarPricePage,
    drawWonPricePage,
    drawBlockPage,
    drawTimePage};
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

void setBitcoinClockBrightnessScheduleEnabled(bool enabled)
{
  brightnessScheduleEnabled = enabled;
  brightnessScheduleOffWindow = false;
  manualBrightnessOverride = false;

  if (!brightnessScheduleEnabled)
    setBitcoinClockBacklightLevel(SCHEDULE_DISABLED_BRIGHTNESS_LEVEL);
}

void setBitcoinClockScheduledBrightness(uint8_t level, bool offWindow)
{
  if (level >= BACKLIGHT_LEVEL_COUNT)
    return;

  brightnessScheduleOffWindow = offWindow;
  manualBrightnessOverride = false;
  setBitcoinClockBacklightLevel(level);
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
