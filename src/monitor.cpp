#include <Arduino.h>
#include <WiFi.h>
#include "mbedtls/md.h"
#include "HTTPClient.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <list>
#include "mining.h"
#include "utils.h"
#include "monitor.h"
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"
#include "timezoneConfig.h"

extern uint32_t templates;
extern uint32_t hashes;
extern uint32_t Mhashes;
extern uint32_t totalKHashes;
extern uint32_t elapsedKHs;
extern uint64_t upTime;

extern uint32_t shares; // increase if blockhash has 32 bits of zeroes
extern uint32_t valids; // increased if blockhash <= targethalfshares

extern double best_diff; // track best diff

extern monitor_data mMonitor;

#ifdef LILYGO_S3_T_DISPLAY
extern void setBitcoinClockBacklightLevel(uint8_t level);
#endif

//from saved config
extern TSettings Settings; 
bool invertColors = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
unsigned int bitcoin_price=0;
String current_block = "------";
String bitcoin_clock_usd_price = "--";
String bitcoin_clock_krw_price = "--";
unsigned long mClockUsdPriceUpdate = 0;
unsigned long mClockKrwPriceUpdate = 0;
constexpr unsigned long BITCOIN_CLOCK_PRICE_REFRESH_MS = 5000;
global_data gData;
pool_data pData;
String poolAPIUrl;
unsigned long mNtpAttempt = 0;
unsigned long mTriggerUpdate = 0;
unsigned long initialTime = 0;


void setup_monitor(void){
#ifdef LILYGO_S3_T_DISPLAY
    const BitcoinClockTimeZone* zone =
        findBitcoinClockTimeZone(Settings.TimezoneName.c_str());
    if (zone == nullptr)
    {
        Serial.printf("[TIME] Unsupported saved time zone '%s'; using %s\n",
                      Settings.TimezoneName.c_str(),
                      DEFAULT_TIMEZONE_NAME);
        Settings.TimezoneName = DEFAULT_TIMEZONE_NAME;
        zone = findBitcoinClockTimeZone(Settings.TimezoneName.c_str());
    }
    setenv("TZ", zone->posixRule, 1);
    tzset();
    Serial.printf("[TIME] Local time zone: %s (%s)\n", zone->name, zone->posixRule);
#endif
    timeClient.begin();

#ifndef LILYGO_S3_T_DISPLAY
    const int timezoneOffsetHours = Settings.Timezone;
    timeClient.setTimeOffset(3600 * timezoneOffsetHours);
#else
    timeClient.setTimeOffset(0);
#endif

    Serial.println("TimeClient setup done");
#ifdef SCREEN_WORKERS_ENABLE
    poolAPIUrl = getPoolAPIUrl();
    Serial.println("poolAPIUrl: " + poolAPIUrl);
#endif
}

unsigned long mGlobalUpdate =0;

void updateGlobalData(void){
    
    if((mGlobalUpdate == 0) || (millis() - mGlobalUpdate > UPDATE_Global_min * 60 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) return;
            
        //Make first API call to get global hash and current difficulty
        HTTPClient http;
        http.setTimeout(10000);
        try {
        http.begin(getGlobalHash);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, payload);
            String temp = "";
            if (doc.containsKey("currentHashrate")) temp = String(doc["currentHashrate"].as<float>());
            if(temp.length()>18 + 3) //Exahashes more than 18 digits + 3 digits decimals
              gData.globalHash = temp.substring(0,temp.length()-18 - 3);
            if (doc.containsKey("currentDifficulty")) temp = String(doc["currentDifficulty"].as<float>());
            if(temp.length()>10 + 3){ //Terahash more than 10 digits + 3 digit decimals
              temp = temp.substring(0,temp.length()-10 - 3);
              gData.difficulty = temp.substring(0,temp.length()-2) + "." + temp.substring(temp.length()-2,temp.length()) + "T";
            }
            doc.clear();

            mGlobalUpdate = millis();
        }
        http.end();

      
        //Make third API call to get fees
        http.begin(getFees);
        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, payload);
            String temp = "";
            if (doc.containsKey("halfHourFee")) gData.halfHourFee = doc["halfHourFee"].as<int>();
#ifdef SCREEN_FEES_ENABLE
            if (doc.containsKey("fastestFee"))  gData.fastestFee = doc["fastestFee"].as<int>();
            if (doc.containsKey("hourFee"))     gData.hourFee = doc["hourFee"].as<int>();
            if (doc.containsKey("economyFee"))  gData.economyFee = doc["economyFee"].as<int>();
            if (doc.containsKey("minimumFee"))  gData.minimumFee = doc["minimumFee"].as<int>();
#endif
            doc.clear();

            mGlobalUpdate = millis();
        }
        
        http.end();
        } catch(...) {
          Serial.println("Global data HTTP error caught");
          http.end();
        }
    }
}

unsigned long mHeightUpdate = 0;
unsigned long mHeightAttempt = 0;

String getBlockHeight(void){
    
    if(((mHeightUpdate == 0) || (millis() - mHeightUpdate > UPDATE_Height_min * 60 * 1000)) &&
       ((mHeightAttempt == 0) || (millis() - mHeightAttempt > 30000))){
    
        if (WiFi.status() != WL_CONNECTED) return current_block;

        mHeightAttempt = millis();
        HTTPClient http;
        http.setTimeout(5000);
        try {
        http.begin(getHeightAPI);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            payload.trim();

            if (payload.length() > 0 && payload.toInt() > 0) {
                current_block = payload;
                mHeightUpdate = millis();
            } else {
                Serial.println("[CLOCK] Block-height API returned invalid data");
            }
        } else {
            Serial.printf("[CLOCK] Block-height request failed: HTTP %d\n", httpCode);
        }        
        http.end();
        } catch(...) {
          Serial.println("Height HTTP error caught");
          http.end();
        }
    }
  
  return current_block;
}

unsigned long mBTCUpdate = 0;

void updateBitcoinClockPrice(void)
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    if (mClockUsdPriceUpdate == 0 ||
        millis() - mClockUsdPriceUpdate >= BITCOIN_CLOCK_PRICE_REFRESH_MS)
    {
        mClockUsdPriceUpdate = millis();
        HTTPClient http;
        http.setTimeout(5000);
        http.begin(getBitcoinClockPriceAPI);
        const int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            StaticJsonDocument<128> doc;
            DeserializationError error = deserializeJson(doc, http.getString());
            if (!error && doc["symbol"] == "BTCUSDT" && doc["price"].is<const char *>()) {
                const float price = doc["price"].as<String>().toFloat();
                if (price > 0.0f) {
                    bitcoin_clock_usd_price = String(static_cast<uint32_t>(price + 0.5f));
                } else {
                    Serial.println("[CLOCK] Binance returned a non-positive BTC price");
                }
            } else if (error) {
                Serial.printf("[CLOCK] Binance price JSON error: %s\n", error.c_str());
            } else {
                Serial.println("[CLOCK] Binance response did not contain a BTCUSDT price");
            }
        } else {
            Serial.printf("[CLOCK] Binance price request failed: HTTP %d\n", httpCode);
        }
        http.end();
    }

    if (mClockKrwPriceUpdate == 0 ||
        millis() - mClockKrwPriceUpdate >= BITCOIN_CLOCK_PRICE_REFRESH_MS)
    {
        mClockKrwPriceUpdate = millis();
        HTTPClient http;
        http.setTimeout(5000);
        http.begin(getBitcoinClockKrwPriceAPI);
        const int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            StaticJsonDocument<64> filter;
            filter[0]["market"] = true;
            filter[0]["trade_price"] = true;
            StaticJsonDocument<128> doc;
            DeserializationError error = deserializeJson(
                doc,
                http.getString(),
                DeserializationOption::Filter(filter));
            JsonArray tickers = doc.as<JsonArray>();

            if (!error && !tickers.isNull() && tickers.size() > 0 &&
                tickers[0]["market"] == "KRW-BTC" &&
                tickers[0]["trade_price"].is<double>()) {
                const double priceKrw = tickers[0]["trade_price"].as<double>();
                if (priceKrw > 0.0) {
                    const uint32_t priceInTenThousandWon =
                        static_cast<uint32_t>(priceKrw / 10000.0 + 0.5);
                    bitcoin_clock_krw_price = String(priceInTenThousandWon);
                } else {
                    Serial.println("[CLOCK] Upbit returned a non-positive BTC price");
                }
            } else if (error) {
                Serial.printf("[CLOCK] Upbit price JSON error: %s\n", error.c_str());
            } else {
                Serial.println("[CLOCK] Upbit response did not contain a KRW-BTC price");
            }
        } else {
            Serial.printf("[CLOCK] Upbit price request failed: HTTP %d\n", httpCode);
        }

        http.end();
    }
}

String getBTCprice(void){
    
    if((mBTCUpdate == 0) || (millis() - mBTCUpdate > UPDATE_BTC_min * 10 * 1000)){
    
        if (WiFi.status() != WL_CONNECTED) {
            static char price_buffer[16];
            snprintf(price_buffer, sizeof(price_buffer), "$%u", bitcoin_price);
            return String(price_buffer);
        }
        
        HTTPClient http;
        http.setTimeout(10000);
        bool priceUpdated = false;

        try {
        http.begin(getBTCAPI);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            StaticJsonDocument<1024> doc;
            deserializeJson(doc, payload);
          
            if (doc.containsKey("bitcoin") && doc["bitcoin"].containsKey("usdt")) {
              bitcoin_price = doc["bitcoin"]["usdt"];
            }

            doc.clear();

            mBTCUpdate = millis();
        }
        
        http.end();
        } catch(...) {
          Serial.println("BTC price HTTP error caught");
          http.end();
        }
    }  
  
  static char price_buffer[16];
  snprintf(price_buffer, sizeof(price_buffer), "$%u", bitcoin_price);
  return String(price_buffer);
}

unsigned long initialMillis = millis();
unsigned long mPoolUpdate = 0;

void getTime(unsigned long* currentHours, unsigned long* currentMinutes, unsigned long* currentSeconds){
  
  //Check if need an NTP call to check current time
  const unsigned long now = millis();
  const bool firstSyncPending = mTriggerUpdate == 0;
  const bool resyncDue = !firstSyncPending &&
                         now - mTriggerUpdate > UPDATE_PERIOD_h * 60UL * 60UL * 1000UL;
  const bool retryDue = mNtpAttempt == 0 || now - mNtpAttempt >= 30000;

  if ((firstSyncPending || resyncDue) && retryDue && WiFi.status() == WL_CONNECTED) {
      mNtpAttempt = now;
      const bool timeUpdated = firstSyncPending ? timeClient.forceUpdate() : timeClient.update();
      if (timeUpdated) {
          mTriggerUpdate = millis();
          initialTime = timeClient.getEpochTime();
          Serial.println("[CLOCK] NTP time synchronized");
      } else {
          Serial.println("[CLOCK] NTP time synchronization failed; retrying in 30 seconds");
      }
    }

  unsigned long elapsedTime = (millis() - mTriggerUpdate) / 1000; // Tiempo transcurrido en segundos
  unsigned long currentTime = initialTime + elapsedTime; // La hora actual

#ifdef LILYGO_S3_T_DISPLAY
  static bool brightnessScheduleInitialized = false;
  static int64_t lastProcessedMinute = -1;
  if (!Settings.BrightnessScheduleEnabled)
  {
    brightnessScheduleInitialized = false;
    lastProcessedMinute = -1;
  }
  else if (mTriggerUpdate != 0)
  {
    const int64_t currentMinute = currentTime / 60;

    if (!brightnessScheduleInitialized || currentMinute != lastProcessedMinute)
    {
      time_t localEpoch = static_cast<time_t>(currentTime);
      struct tm localTime;
      if (localtime_r(&localEpoch, &localTime) != nullptr)
      {
        const int minuteOfDay = localTime.tm_hour * 60 + localTime.tm_min;
        const bool weekend = localTime.tm_wday == 0 || localTime.tm_wday == 6;
        uint8_t brightnessLevel;

        if (minuteOfDay < 330)
          brightnessLevel = 0;
        else if (minuteOfDay < 480)
          brightnessLevel = 1;
        else if (!weekend && minuteOfDay < 660)
          brightnessLevel = 2;
        else if (!weekend && minuteOfDay < 1020)
          brightnessLevel = 0;
        else
          brightnessLevel = 2;

        const bool isScheduleStart = minuteOfDay == 0 ||
                                     minuteOfDay == 330 ||
                                     minuteOfDay == 480 ||
                                     (!weekend && (minuteOfDay == 660 || minuteOfDay == 1020));
        if (!brightnessScheduleInitialized || isScheduleStart)
        {
          setBitcoinClockBacklightLevel(brightnessLevel);
          Serial.printf("[DISPLAY] Automatic brightness: %u%% (%s %02d:%02d)\n",
                        brightnessLevel == 0 ? 0 : brightnessLevel == 1 ? 25 : 50,
                        weekend ? "weekend" : "weekday",
                        localTime.tm_hour,
                        localTime.tm_min);
        }

        brightnessScheduleInitialized = true;
        lastProcessedMinute = currentMinute;
      }
    }
  }
#endif

  // Convert the synchronized UTC epoch to the selected local time.
#ifdef LILYGO_S3_T_DISPLAY
  time_t localEpoch = static_cast<time_t>(currentTime);
  struct tm localTime;
  if (mTriggerUpdate != 0 && localtime_r(&localEpoch, &localTime) != nullptr)
  {
    *currentHours = localTime.tm_hour;
    *currentMinutes = localTime.tm_min;
    *currentSeconds = localTime.tm_sec;
  }
  else
  {
    *currentHours = *currentMinutes = *currentSeconds = 0;
  }
#else
  *currentHours = currentTime % 86400 / 3600;
  *currentMinutes = currentTime % 3600 / 60;
  *currentSeconds = currentTime % 60;
#endif
}

String getDate(){
  
  unsigned long elapsedTime = (millis() - mTriggerUpdate) / 1000; // Tiempo transcurrido en segundos
  unsigned long currentTime = initialTime + elapsedTime; // La hora actual

  // Convierte la hora actual (epoch time) en una estructura tm
  time_t localEpoch = static_cast<time_t>(currentTime);
  struct tm localTime;
  if (localtime_r(&localEpoch, &localTime) == nullptr)
    return "";
  struct tm *tm = &localTime;

  int year = tm->tm_year + 1900; // tm_year es el número de años desde 1900
  int month = tm->tm_mon + 1;    // tm_mon es el mes del año desde 0 (enero) hasta 11 (diciembre)
  int day = tm->tm_mday;         // tm_mday es el día del mes

  char currentDate[20];
  sprintf(currentDate, "%02d/%02d/%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);

  return String(currentDate);
}

String getTime(void){
  unsigned long currentHours, currentMinutes, currentSeconds;
  getTime(&currentHours, &currentMinutes, &currentSeconds);
  if (mTriggerUpdate == 0) return "--:--";

  char LocalHour[10];
  sprintf(LocalHour, "%02d:%02d", currentHours, currentMinutes);
  
  String mystring(LocalHour);
  return LocalHour;
}

bitcoin_clock_data getBitcoinClockData(void)
{
  bitcoin_clock_data data;
  data.blockHeight = getBlockHeight();
  updateBitcoinClockPrice();
  data.btcPriceUsd = bitcoin_clock_usd_price;
  data.btcPriceKrw = bitcoin_clock_krw_price;
  data.currentTime = getTime();
  return data;
}

enum EHashRateScale
{
  HashRateScale_99KH,
  HashRateScale_999KH,
  HashRateScale_9MH
};

static EHashRateScale s_hashrate_scale = HashRateScale_99KH;
static uint32_t s_skip_first = 3;
static double s_top_hashrate = 0.0;

static std::list<double> s_hashrate_avg_list;
static double s_hashrate_summ = 0.0;
static uint8_t s_hashrate_recalc = 0;

String getCurrentHashRate(unsigned long mElapsed)
{
  double hashrate = (double)elapsedKHs * 1000.0 / (double)mElapsed;

  s_hashrate_summ += hashrate;
  s_hashrate_avg_list.push_back(hashrate);
  if (s_hashrate_avg_list.size() > 10)
  {
    s_hashrate_summ -= s_hashrate_avg_list.front();
    s_hashrate_avg_list.pop_front();
  }

  ++s_hashrate_recalc;
  if (s_hashrate_recalc == 0)
  {
    s_hashrate_summ = 0.0;
    for (auto itt = s_hashrate_avg_list.begin(); itt != s_hashrate_avg_list.end(); ++itt)
      s_hashrate_summ += *itt;
  }

  double avg_hashrate = s_hashrate_summ / (double)s_hashrate_avg_list.size();
  if (avg_hashrate < 0.0)
    avg_hashrate = 0.0;

  if (s_skip_first > 0)
  {
    s_skip_first--;
  } else
  {
    if (avg_hashrate > s_top_hashrate)
    {
      s_top_hashrate = avg_hashrate;
      if (avg_hashrate > 999.9)
        s_hashrate_scale = HashRateScale_9MH;
      else if (avg_hashrate > 99.9)
        s_hashrate_scale = HashRateScale_999KH;
    }
  }

  switch (s_hashrate_scale)
  {
    case HashRateScale_99KH:
      return String(avg_hashrate, 2);
    case HashRateScale_999KH:
      return String(avg_hashrate, 1);
    default:
      return String((int)avg_hashrate );
  }
}

mining_data getMiningData(unsigned long mElapsed)
{
  mining_data data;

  char best_diff_string[16] = {0};
  suffix_string(best_diff, best_diff_string, 16, 0);

  char timeMining[15] = {0};
  uint64_t tm = upTime;
  int secs = tm % 60;
  tm /= 60;
  int mins = tm % 60;
  tm /= 60;
  int hours = tm % 24;
  int days = tm / 24;
  sprintf(timeMining, "%01d  %02d:%02d:%02d", days, hours, mins, secs);

  data.completedShares = shares;
  data.totalMHashes = Mhashes;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.templates = templates;
  data.bestDiff = best_diff_string;
  data.timeMining = timeMining;
  data.valids = valids;
  data.temp = String(temperatureRead(), 0);
  data.currentTime = getTime();

  return data;
}

clock_data getClockData(unsigned long mElapsed)
{
  clock_data data;

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = getBTCprice();
  data.blockHeight = getBlockHeight();
  data.currentTime = getTime();
  data.currentDate = getDate();

  return data;
}

clock_data_t getClockData_t(unsigned long mElapsed)
{
  clock_data_t data;

  data.valids = valids;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  getTime(&data.currentHours, &data.currentMinutes, &data.currentSeconds);

  return data;
}

coin_data getCoinData(unsigned long mElapsed)
{
  coin_data data;

  updateGlobalData(); // Update gData vars asking mempool APIs

  data.completedShares = shares;
  data.totalKHashes = totalKHashes;
  data.currentHashRate = getCurrentHashRate(mElapsed);
  data.btcPrice = getBTCprice();
  data.currentTime = getTime();
#ifdef SCREEN_FEES_ENABLE
  data.hourFee = String(gData.hourFee);
  data.fastestFee = String(gData.fastestFee);
  data.economyFee = String(gData.economyFee);
  data.minimumFee = String(gData.minimumFee);
#endif
  data.halfHourFee = String(gData.halfHourFee) + " sat/vB";
  data.netwrokDifficulty = gData.difficulty;
  data.globalHashRate = gData.globalHash;
  data.blockHeight = getBlockHeight();

  unsigned long currentBlock = data.blockHeight.toInt();
  unsigned long remainingBlocks = (((currentBlock / HALVING_BLOCKS) + 1) * HALVING_BLOCKS) - currentBlock;
  data.progressPercent = (HALVING_BLOCKS - remainingBlocks) * 100 / HALVING_BLOCKS;
  data.remainingBlocks = String(remainingBlocks) + " BLOCKS";

  return data;
}

String getPoolAPIUrl(void) {
    poolAPIUrl = String(getPublicPool);
    if (Settings.PoolAddress == "public-pool.io") {
        poolAPIUrl = "https://public-pool.io:40557/api/client/";
    } 
    else {
        if (Settings.PoolAddress == "pool.nerdminers.org") {
            poolAPIUrl = "https://pool.nerdminers.org/users/";
        }
        else {
            switch (Settings.PoolPort) {
                case 3333:
                    if (Settings.PoolAddress == "pool.sethforprivacy.com")
                        poolAPIUrl = "https://pool.sethforprivacy.com/api/client/";
                    if (Settings.PoolAddress == "pool.solomining.de")
                        poolAPIUrl = "https://pool.solomining.de/api/client/";
                    // Add more cases for other addresses with port 3333 if needed
                    break;
                case 2018:
                    // Local instance of public-pool.io on Umbrel or Start9
                    poolAPIUrl = "http://" + Settings.PoolAddress + ":2019/api/client/";
                    break;
                default:
                    poolAPIUrl = String(getPublicPool);
                    break;
            }
        }
    }
    return poolAPIUrl;
}

pool_data getPoolData(void){
    //pool_data pData;    
    if((mPoolUpdate == 0) || (millis() - mPoolUpdate > UPDATE_POOL_min * 60 * 1000)){      
        if (WiFi.status() != WL_CONNECTED) return pData;            
        //Make first API call to get global hash and current difficulty
        HTTPClient http;
        http.setTimeout(10000);        
        try {          
          String btcWallet = Settings.BtcWallet;
          // Serial.println(btcWallet);
          if (btcWallet.indexOf(".")>0) btcWallet = btcWallet.substring(0,btcWallet.indexOf("."));
#ifdef SCREEN_WORKERS_ENABLE
          Serial.println("Pool API : " + poolAPIUrl+btcWallet);
          http.begin(poolAPIUrl+btcWallet);
#else
          http.begin(String(getPublicPool)+btcWallet);
#endif
          int httpCode = http.GET();
          if (httpCode == HTTP_CODE_OK) {
              String payload = http.getString();
              // Serial.println(payload);
              StaticJsonDocument<300> filter;
              filter["bestDifficulty"] = true;
              filter["workersCount"] = true;
              filter["workers"][0]["sessionId"] = true;
              filter["workers"][0]["hashRate"] = true;
              StaticJsonDocument<2048> doc;
              deserializeJson(doc, payload, DeserializationOption::Filter(filter));
              //Serial.println(serializeJsonPretty(doc, Serial));
              if (doc.containsKey("workersCount")) pData.workersCount = doc["workersCount"].as<int>();
              const JsonArray& workers = doc["workers"].as<JsonArray>();
              float totalhashs = 0;
              for (const JsonObject& worker : workers) {
                totalhashs += worker["hashRate"].as<double>();
                /* Serial.print(worker["sessionId"].as<String>()+": ");
                Serial.print(" - "+worker["hashRate"].as<String>()+": ");
                Serial.println(totalhashs); */
              }
              char totalhashs_s[16] = {0};
              suffix_string(totalhashs, totalhashs_s, 16, 0);
              pData.workersHash = String(totalhashs_s);

              double temp;
              if (doc.containsKey("bestDifficulty")) {
              temp = doc["bestDifficulty"].as<double>();            
              char best_diff_string[16] = {0};
              suffix_string(temp, best_diff_string, 16, 0);
              pData.bestDifficulty = String(best_diff_string);
              }
              doc.clear();
              mPoolUpdate = millis();
              Serial.println("\n####### Pool Data OK!");               
          } else {
              Serial.println("\n####### Pool Data HTTP Error!");    
              /* Serial.println(httpCode);
              String payload = http.getString();
              Serial.println(payload); */
              // mPoolUpdate = millis();
              pData.bestDifficulty = "P";
              pData.workersHash = "E";
              pData.workersCount = 0;
              http.end();
              return pData; 
          }
          http.end();
        } catch(...) {
          Serial.println("####### Pool Error!");          
          // mPoolUpdate = millis();
          pData.bestDifficulty = "P";
          pData.workersHash = "Error";
          pData.workersCount = 0;
          http.end();
          return pData;
        } 
    }
    return pData;
}
