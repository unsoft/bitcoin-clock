
#include <Wire.h>

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <OneButton.h>

#include "wManager.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "timeconst.h"

#ifdef TOUCH_ENABLE
#include "TouchHandler.h"
#endif

#ifdef PIN_BUTTON_1
  OneButton button1(PIN_BUTTON_1);
#endif

#ifdef PIN_BUTTON_2
  OneButton button2(PIN_BUTTON_2);
#endif

#ifdef TOUCH_ENABLE
extern TouchHandler touchHandler;
#endif

extern monitor_data mMonitor;

#ifdef SD_ID
  SDCard SDCrd = SDCard(SD_ID);
#else  
  SDCard SDCrd = SDCard();
#endif

/**********************⚡ GLOBAL Vars *******************************/

unsigned long start = millis();
const char* ntpServer = "pool.ntp.org";

//void runMonitor(void *name);


/********* INIT *****/
void setup()
{
      //Init pin 15 to eneble 5V external power (LilyGo bug)
      //Also used as power-hold latch on M5StickC Plus2 (GPIO4)
  #ifdef PIN_ENABLE5V
      pinMode(PIN_ENABLE5V, OUTPUT);
      digitalWrite(PIN_ENABLE5V, HIGH);
  #endif

#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED

  Serial.setTimeout(0);
  delay(SECOND_MS/10);

  // Setup the buttons
  #if defined(PIN_BUTTON_1) && !defined(PIN_BUTTON_2) //One button device
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(switchToNextScreen);
    button1.attachDoubleClick(alternateScreenRotation);
    button1.attachLongPressStart(reset_configuration);
    button1.attachMultiClick(alternateScreenState);
  #endif

  #if defined(PIN_BUTTON_1) && defined(PIN_BUTTON_2) //Button 1 of two button device
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(switchToNextScreen);
    button1.attachDoubleClick(alternateScreenRotation);
  #endif

  #if defined(PIN_BUTTON_2) //Button 2 of two button device
    button2.setPressMs(5*SECOND_MS);
    button2.attachClick(alternateScreenState);
    button2.attachLongPressStart(reset_configuration);
  #endif

  /******** INIT BITCOIN CLOCK ************/
  Serial.println("Bitcoin Clock starting...");

  /******** INIT DISPLAY ************/
  initDisplay();
  
  /******** PRINT INIT SCREEN *****/
  drawLoadingScreen();
  delay(2*SECOND_MS);

  /******** SHOW LED INIT STATUS (devices without screen) *****/
  mMonitor.NerdStatus = NM_waitingConfig;
  doLedStuff(0);

#ifdef SDMMC_1BIT_FIX
  SDCrd.initSDcard();
#endif

  /******** INIT WIFI ************/
  init_WifiManager();

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  static const char monitor_name[] = "(Monitor)";
  #if defined(CONFIG_IDF_TARGET_ESP32)
  // Increased stack for ESP32 classic due to NVS operations  
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 9500, (void*)monitor_name, 5, NULL,1);
  #else
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 10000, (void*)monitor_name, 5, NULL,1);
  #endif

  /******** MONITOR SETUP *****/
  setup_monitor();
}

void app_error_fault_handler(void *arg) {
  // Get stack errors
  char *stack = (char *)arg;

  // Print the stack errors in the console
  esp_log_write(ESP_LOG_ERROR, "APP_ERROR", "Error Stack Code:\n%s", stack);

  // restart ESP32
  esp_restart();
}

void loop() {
  // keep watching the push buttons:
  #ifdef PIN_BUTTON_1
    button1.tick();
  #endif

  #ifdef PIN_BUTTON_2
    button2.tick();
  #endif

#ifdef TOUCH_ENABLE
  touchHandler.isTouched();
#endif
  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code

  vTaskDelay(50 / portTICK_PERIOD_MS);
}
