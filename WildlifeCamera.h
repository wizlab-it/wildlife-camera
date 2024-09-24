/**
 * @package Wildlife Camera
 * Main Code header
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240811.041
 */

#ifndef WILDLIFECAMERA_H
#define WILDLIFECAMERA_H


/**
 * Defines
 */
//Built-in Led
#define _LED_PIN    GPIO_NUM_33

//WiFi connection timeout (in seconds)
#define _WIFI_CONNECTION_TIMEOUT 15

//Timings (in seconds)
#define _WAKEUP_DURATION_DEFAULT       30
#define _WAKEUP_DURATION_BY_TIMER      5
#define _WAKEUP_DURATION_BY_PIR        60
#define _WAKEUP_INCREASE_BY_TELEGRAM   60
#define _DEEP_SLEEP_DURATION           600


/**
 * Includes
 */
#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "soc/rtc.h"
#include "driver/rtc_io.h"
#include "config.h"
#include "pir.h"
#include "camera.h"
#include "telegram.h"


/**
 * Structs
 */
//PIR
struct {
  bool motionDetected = false;
  uint8_t *photo = NULL;
  long photoLength = 0;
} __PIR;

//Low Battery (data kept across deep sleeps)
RTC_DATA_ATTR struct {
  unsigned long batteryVoltageCacheExpire = 0;
  uint32_t batteryVoltageRaw = 0;
  uint32_t batteryVoltageMillivoltsOnAnalogPin = 0;
  uint32_t batteryVoltageMillivoltsEffective = 0;
  uint8_t batteryLastNotificationLevel = 5;
  unsigned long startupTimestamp = 0;
} __System;

//Wake Up
struct {
  esp_sleep_wakeup_cause_t reason;
  unsigned long end = 0;
} __WakeUp;

//Timers
struct {
  unsigned long wifiConnectionTimeout = 0;
  unsigned long telegramGetUpdates = 0;
} __Timers;


/**
 * Functions
 */
unsigned long setWakeupEnd(unsigned long increase);
unsigned long getTimestamp();
unsigned long getUptime();
String getDateFormat(String format, time_t timestamp);
uint32_t getBatteryVoltage(bool getRaw);
uint8_t getBatteryLevel();
float cameraSdGetUsedSpace();
void telegramCommandProcessor(const char* command);


#endif