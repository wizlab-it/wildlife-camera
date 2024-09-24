/**
 * @package Wildlife Camera
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240826.126
 */

#include "WildlifeCamera.h"


/**
 * Global variables
 * Initialize Telegram communications and Camera
 */
Pir pir(PIR_ENABLED, PIR_PIN);
Telegram telegram(TELEGRAM_BOT_API_TOKEN, TELEGRAM_CHAT_ID, &telegramCommandProcessor);
Camera camera(CAMERA_FRAME_SIZE, CAMERA_QUALITY, CAMERA_SDCARD_ENABLED);


/**
 * PIR interrupt function
 */
void IRAM_ATTR pirInterrupt() {
  __PIR.motionDetected = true;
}


/**
 * Setup
 */
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  //Start serial
  Serial.begin(115200);
  Serial.println(F("\n\n\n\n\n"));
  Serial.println(F("************************************************************************"));
  Serial.println(F("***               ~  Wildlife Camera - by WizLab.it  ~               ***"));
  Serial.println(F("************************************************************************"));
  Serial.println(F("[~~~~~] Setup:"));

  //Get wake up reason
  __WakeUp.reason = deepSleepWakeUpCheck();

  //Initialize camera and check if to take a photo (wake up by PIR)
  bool cameraStatus = camera.init();
  if(cameraStatus && (__WakeUp.reason == ESP_SLEEP_WAKEUP_EXT0)) {
    Serial.print(" [*] Take photo after PIR wake up... ");
    delay(1500);
    Serial.println("now");
    __PIR.photoLength = camera.takePhoto(&__PIR.photo, false);
  }

  //Configure built-in led
  pinMode(_LED_PIN, OUTPUT);
  digitalWrite(_LED_PIN, HIGH);

  //Configure lob-battery input pin
  pinMode(_LOWBATTERY_PIN, INPUT);

  //Connect to Wi-Fi
  wifiConnect(true);

  //Check if camera is available
  if(cameraStatus) {
    Serial.println(" [+] Camera activated");
    camera.sdOpen(); //Initiallize SD Card
    camera.sdClose();
  } else {
    Serial.println(" [-] Camera initialization");
    Serial.println(F("[~~~~~] Going to deep sleep..."));
    telegram.sendMessage("Camera initialization failed, going to sleep");
    deepSleepActivate(_DEEP_SLEEP_DURATION, true);
  }

  //Set wake up duration based on wake up reason
  switch(__WakeUp.reason) {
    case ESP_SLEEP_WAKEUP_EXT0: setWakeupEnd(_WAKEUP_DURATION_BY_PIR); break;
    case ESP_SLEEP_WAKEUP_TIMER: setWakeupEnd(_WAKEUP_DURATION_BY_TIMER); break;
    default: setWakeupEnd(_WAKEUP_DURATION_DEFAULT); break;
  }

  //Enable PIR
  if(PIR_ENABLED) pir.enable(&pirInterrupt);

  //Setup complete
  Serial.println(F("[~~~~~] Setup complete."));
  Serial.println(F("[~~~~~] Running:"));
}


/**
 * Loop
 */
void loop() {
  //If no photos are stored and PIR is enabled and motion is detected, then takes a new photo and calculate new wake up duration
  if(PIR_ENABLED && (__PIR.photo == NULL) && __PIR.motionDetected) {
    Serial.println(" [i] Motion detected, taking photo");
    __PIR.photoLength = camera.takePhoto(&__PIR.photo, false);
    setWakeupEnd(_WAKEUP_DURATION_BY_PIR);
    __PIR.motionDetected = false;
  }

  //Check WiFi connection
  if(wifiConnect(false)) {
    //Check if there is a photo to be sent on telegram
    if((__PIR.photoLength > 0) && (__PIR.photo != NULL)) {
      telegram.sendPhoto(__PIR.photo, __PIR.photoLength);
    }

    //Check Telegram updates every 5 seconds
    if(__Timers.telegramGetUpdates < millis()) {
      __Timers.telegramGetUpdates = millis() + 5000;
 
      //int8_t telegramUpdatesCount = telegramGetUpdates();
      int8_t telegramUpdatesCount = telegram.getUpdates();

      //If there are updates, then recalculate wake up duration
      if(telegramUpdatesCount > 0) setWakeupEnd(_WAKEUP_INCREASE_BY_TELEGRAM);
    }
  }

  //Battery level check
  batteryCheck();

  //Free PIR photo
  free(__PIR.photo);
  __PIR.photo = NULL;
  __PIR.photoLength = 0;

  //Check if go to deep sleep
  if(millis() > __WakeUp.end) {
    Serial.println(F("[~~~~~] Going to deep sleep..."));
    deepSleepActivate(_DEEP_SLEEP_DURATION, true);
  }

  //Loop end
  delay(500);
}


/**
 * wifiConnect
 * Connect to WiFi Network
 * @param blocking    if true, wait for connection result; if false, return immediately
 * @return            true if connection is successful, false otherwise
 */
bool wifiConnect(bool blocking) {
  bool wifiConnectionJustInitiated = false;

  //Check if already connected
  if(WiFi.status() == WL_CONNECTED) {
    //Get current timestamp
    long timestamp = getTimestamp();

    //Check if NTP is OK
    if(timestamp < 1000000000) {
      configTime((NTP_TIMEZONE * 3600), 3600, NTP_SERVER);
      timestamp = getTimestamp();
    }

    //Set startup timestamp
    if(__System.startupTimestamp == 0) {
      if(timestamp > 1000000000) {
        __System.startupTimestamp = timestamp - ((unsigned long)(millis() / 1000));
      }
    }

    return true;
  }

  //If here, it's not connected: if WiFi is OFF, then reset and start WiFi Connection
  wl_status_t wifiStatus = WiFi.status();
  if((wifiStatus != WL_NO_SHIELD) || (wifiStatus == WL_STOPPED)) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(250);
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    wifiConnectionJustInitiated = true;
  }

  //Try to connect
  if(blocking) {
    Serial.printf(" [*] WiFi: connecting to %s: ", WIFI_SSID);
    uint8_t i = 0;
    while(WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      if(i++ > _WIFI_CONNECTION_TIMEOUT) break;
      delay(1000);
    }
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println(" OK");
    } else {
      Serial.println(" failed");
      WiFi.disconnect(true);
    }
  } else {
    if(wifiConnectionJustInitiated) {
      __Timers.wifiConnectionTimeout = millis() + (_WIFI_CONNECTION_TIMEOUT * 1000);
      Serial.printf(" [*] WiFi: try to connect to %s (non-blocking)\n", WIFI_SSID);
    } else {
      if((WiFi.status() == WL_DISCONNECTED)) { //Connecting
        if(__Timers.wifiConnectionTimeout < millis()) {
          Serial.printf(" [-] WiFi: connection to %s failed\n", WIFI_SSID);
          WiFi.disconnect(true);
        }
      }
    }
  }

  //Check connection status
  if(WiFi.status() == WL_CONNECTED) {
    Serial.printf(" [+] WiFi: connected to %s (%s)\n", WIFI_SSID, WiFi.localIP().toString().c_str());

    //Set local time via NTP
    configTime((NTP_TIMEZONE * 3600), 3600, NTP_SERVER);

    return true;
  }

  //If here, connection failed
  return false;
}


/**
 * setWakeupEnd
 * Set new wake up end
 * End is changed only if larger than the current one
 * @param increase  How much the wake up duration should be increased
 * @return          Wake up end set
 */
unsigned long setWakeupEnd(unsigned long increase) {
  unsigned long newEnd = millis() + (increase * 1000);
  if(newEnd > __WakeUp.end) {
    __WakeUp.end = newEnd;
  }
  return __WakeUp.end;
}


/**
 * getTimestamp
 * Get current timestamp
 * @return    Current timestamp
 */
unsigned long getTimestamp() {
  struct tm timeinfo;
  time_t now;

  //Check NTP status
  if(!getLocalTime(&timeinfo)) {
    return (long)(millis() / 1000);
  }

  //Get current timestamp
  time(&now);
  return (now > 100000000) ? now : (long)(millis() / 1000);
}


/**
 * getDateFormat
 * Get formatted date
 * @param format      Format as defined for strftime()
 * @param timestamp   (optional) If set, use this timestamp, otherwise use current timestamp
 * @return            Formatted date as String
 */
String getDateFormat(String format, time_t timestamp) {
  struct tm timeinfo;
  time_t now;
  char datetime[50];

  //Check NTP status
  if(!getLocalTime(&timeinfo)) {
    return String("");
  }

  //Set time
  if(timestamp != 0) {
    now = timestamp;
  } else {
    time(&now);
    if(now < 100000000) return String("");
  }
  timeinfo = *localtime(&now);

  //Build string
  strftime(datetime, 50, format.c_str(), &timeinfo);
  return String(datetime);
}


/**
 * getUptime
 * Get uptime
 * @return    Uptime in seconds
 */
unsigned long getUptime() {
  if(__System.startupTimestamp != 0) {
    return (getTimestamp() - __System.startupTimestamp);
  } else {
    return (unsigned long)(millis() / 1000);
  }
}


/**
 * batteryCheck
 * General battery status check
 */
void batteryCheck() {
  //Battery level check (also update __System battery level variables)
  uint8_t batteryLevel = getBatteryLevel();

  if(batteryLevel < __System.batteryLastNotificationLevel) {
    Serial.println(" [*] Battery level: " + String(batteryLevel) + "/5");
    int8_t telegramStatus = telegram.sendMessage("Wildlife Camera battery status\n"
      " [+] Battery level: " + String(batteryLevel) + "/5 (" + String(_LOWBATTERY_NUMBER_OF_BATTERIES) + " batteries)\n"
      " [+] PIN voltage: " + String(__System.batteryVoltageMillivoltsOnAnalogPin / 1000.0) + "V\n"
      " [+] Pack voltage: " + String(__System.batteryVoltageMillivoltsEffective / 1000.0) + "V\n"
      " [+] Single voltage: " + String(__System.batteryVoltageMillivoltsEffective / (1000.0 * _LOWBATTERY_NUMBER_OF_BATTERIES)) + "V\n");
    if(telegramStatus == 0) __System.batteryLastNotificationLevel = batteryLevel;
  }

  //If battery is critically low, disable PIR and go to sleep for long time
  if(batteryLevel == 0) {
    Serial.println(" [~~~~~] Battery level critically low!\nSleeping for " + String(_LOWBATTERY_CRITICAL_DEEP_SLEEP) + " hours.");
    telegram.sendMessage("Wildlife Camera battery level critically low!\n"
      " [+] Battery level: " + String(batteryLevel) + "/5 (" + String(_LOWBATTERY_NUMBER_OF_BATTERIES) + " batteries)\n"
      " [+] PIN voltage: " + String(__System.batteryVoltageMillivoltsOnAnalogPin / 1000.0) + "V\n"
      " [+] Pack voltage: " + String(__System.batteryVoltageMillivoltsEffective / 1000.0) + "V\n"
      " [+] Single voltage: " + String(__System.batteryVoltageMillivoltsEffective / (1000.0 * _LOWBATTERY_NUMBER_OF_BATTERIES)) + "V\n"
      "Sleeping for " + String(_LOWBATTERY_CRITICAL_DEEP_SLEEP) + " hours.");
    deepSleepActivate(_LOWBATTERY_CRITICAL_DEEP_SLEEP * 60 * 60, false);
  }
}


/**
 * getBatteryVoltage
 * Get the battery voltage
 * @param getRaw    (optional, default true) If true, then return raw ADC result; if false, then returns millivolt
 * @return          Battery voltage (raw ADC or millivolts)
 */
uint32_t getBatteryVoltage(bool getRaw) {
  //Check if battery ADC needs to be updated
  if(__System.batteryVoltageCacheExpire < getTimestamp()) {
    __System.batteryVoltageCacheExpire = getTimestamp() + _LOWBATTERY_CACHE_TIMEOUT;

    //New battery ADC sample (disable WiFi when sampling)
    WiFi.disconnect(true);
    delay(1000);
    __System.batteryVoltageRaw = analogRead(_LOWBATTERY_PIN);
    delay(500);
    __System.batteryVoltageMillivoltsOnAnalogPin = analogReadMilliVolts(_LOWBATTERY_PIN);
    __System.batteryVoltageMillivoltsEffective = __System.batteryVoltageMillivoltsOnAnalogPin * _LOWBATTERY_VDIV_RATIO;  //Calculate effective battery voltage after voltage divider
    Serial.printf(" [i] Single battery voltage: %0.2fV (original: %0.2fV; raw: %d) (next sample on %s)\n", (__System.batteryVoltageMillivoltsEffective / (1000.0 * _LOWBATTERY_NUMBER_OF_BATTERIES)), (__System.batteryVoltageMillivoltsOnAnalogPin / (1000.0 * _LOWBATTERY_NUMBER_OF_BATTERIES)), (__System.batteryVoltageRaw / _LOWBATTERY_NUMBER_OF_BATTERIES), getDateFormat("%F, %T", __System.batteryVoltageCacheExpire).c_str());
    Serial.printf(" [i] %d-pack battery voltage: %0.2fV (original: %0.2fV; raw: %d) (next sample on %s)\n", _LOWBATTERY_NUMBER_OF_BATTERIES, (__System.batteryVoltageMillivoltsEffective / 1000.0), (__System.batteryVoltageMillivoltsOnAnalogPin / 1000.0), __System.batteryVoltageRaw, getDateFormat("%F, %T", __System.batteryVoltageCacheExpire).c_str());
    wifiConnect(true);

    //Set new wakeup duration
    setWakeupEnd(_WAKEUP_DURATION_DEFAULT);
  }

  return (getRaw) ? __System.batteryVoltageRaw : __System.batteryVoltageMillivoltsEffective;
}


/**
 * getBatteryLevel
 * Get battery level (0 to 5)
 * @return      Battery level (0-5)
 */
uint8_t getBatteryLevel() {
  uint32_t batteryVoltageMillivolts = getBatteryVoltage(false);

  //If battery voltage is <6000 mV, then it's connected to USB
  if(batteryVoltageMillivolts < (3000 * _LOWBATTERY_NUMBER_OF_BATTERIES)) return 5;

  //Battery Powered
  if(batteryVoltageMillivolts > (4000 * _LOWBATTERY_NUMBER_OF_BATTERIES)) return 5;
  else if(batteryVoltageMillivolts > (3900 * _LOWBATTERY_NUMBER_OF_BATTERIES)) return 4;
  else if(batteryVoltageMillivolts > (3800 * _LOWBATTERY_NUMBER_OF_BATTERIES)) return 3;
  else if(batteryVoltageMillivolts > (3700 * _LOWBATTERY_NUMBER_OF_BATTERIES)) return 2;
  else if(batteryVoltageMillivolts > (3600 * _LOWBATTERY_NUMBER_OF_BATTERIES)) return 1;
  else return 0;
}


/**
 * deepSleepActivate
 * Activate deep sleep
 * @param seconds             seconds to deep sleep before to wake up
 * @param enableWakeupByPir   if true, wake up by PIR is enabled; if false PIR won't wake up the device
 */
void deepSleepActivate(uint16_t seconds, bool enableWakeupByPir) {
  //Hold flash when sleeping
  camera.flashGpioHold(true);

  //PIR wake up
  pir.prepareDeepSleep(enableWakeupByPir);

  //Timer wake up
  esp_sleep_enable_timer_wakeup(seconds * 1000 * 1000);

  //Go to deep sleep
  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
}


/**
 * deepSleepWakeUpCheck
 * Check if wake up from deep sleep
 * @return    Wake up reason
 */
esp_sleep_wakeup_cause_t deepSleepWakeUpCheck() {
  //Disable pin hold
  camera.flashGpioHold(false);
  gpio_deep_sleep_hold_dis();

  //Get wake up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: //Wake up from PIR
      Serial.println(" [*] Wake up by PIR (EXT0)");
      break;
    case ESP_SLEEP_WAKEUP_TIMER: //Wake up from TIMER
      Serial.println(" [*] Wake up by TIMER");
      break;
  }

  return wakeup_reason;
}


/**
 * cameraSdGetUsedSpace
 * Camera::sdGetUsedSpace() wrapper
 * @return    SD card usage percentage
 */
float cameraSdGetUsedSpace() {
  return camera.sdGetUsedSpace();
}


/**
 * telegramCommandProcessor
 * External function that processes the received telegram commands
 * ----------------------------------------
 * Telegram Bot commands list configuration
 * wakeup - Wake up from deep sleep
 * photo - Take a photo
 * photoflash - Take a photo with flash
 * status - Device status
 * blink - Blink flash (identify device)
 * ----------------------------------------
 * @param command     command string (i.e. /status)
 */
void telegramCommandProcessor(const char* command) {
  //Command: /wakeup
  //Send wake up welcome message
  if(strcmp("/wakeup", command) == 0) {
    setWakeupEnd(_WAKEUP_INCREASE_BY_TELEGRAM * 5);
    telegram.sendMessage("Hello!\nCurrent date and time: " + getDateFormat("%F, %T") + "\n\nI'll be awake for " + String(_WAKEUP_INCREASE_BY_TELEGRAM * 5 / 60) + " minutes, please send your commands");
  }

  //Command: /photo
  //Takes a photo and sent it to the telegram chat
  else if(strcmp("/photo", command) == 0) {
    uint8_t *photo = NULL;
    long photoLength = camera.takePhoto(&photo, false);
    if(photoLength > 0) telegram.sendPhoto(photo, photoLength);
    free(photo);
    photo = NULL;
  }

  //Command: /photoflash
  //Takes a photo with flash and sent it to the telegram chat
  else if(strcmp("/photoflash", command) == 0) {
    uint8_t *photo = NULL;
    long photoLength = camera.takePhoto(&photo, true);
    if(photoLength > 0) telegram.sendPhoto(photo, photoLength);
    free(photo);
    photo = NULL;
  }

  //Command: /status
  //Send device status
  else if(strcmp("/status", command) == 0) {
    String datetime = getDateFormat("%F, %T");
    String statusMessage = "Wildlife Camera by WizLab.it\n";

    //Calculate uptime parts
    unsigned long uptimeSeconds = getUptime();
    uint16_t uptimeHours = uint16_t(uptimeSeconds / 3600);
    uptimeSeconds -= (uptimeHours * 3600);
    uint8_t uptimeMinutes = uint8_t(uptimeSeconds / 60);
    uptimeSeconds -= (uptimeMinutes * 60);

    //Update battery voltages
    getBatteryVoltage(false);

    //Device status
    statusMessage += "\nDevice:\n"
      " [" + String((datetime == "") ? "-" : "+") + "] Date and time: " + ((datetime == "") ? "unknown" : datetime) + "\n"
      " [+] Uptime: " + String(uptimeHours) + " hours, " + String(uptimeMinutes) + " minutes, " + String(uptimeSeconds) + " seconds\n"
      " [+] Device ID: " + String(ESP.getEfuseMac()) + "\n";

    //WiFi status
    statusMessage += "\nWiFi:\n"
      " [+] SSID: " + WiFi.SSID() + "\n"
      " [+] RSSI: " + String(WiFi.RSSI()) + "\n"
      " [+] IP Address: " + String(WiFi.localIP().toString()) + "\n";

    //Battery status
    statusMessage += "\nBattery:\n"
      " [+] Level: " + String(getBatteryLevel()) + "/5\n"
      " [+] Quantity: " + String(_LOWBATTERY_NUMBER_OF_BATTERIES) + "\n"
      " [+] PIN voltage: " + String(__System.batteryVoltageMillivoltsOnAnalogPin / 1000.0) + "V\n"
      " [+] Pack voltage: " + String(__System.batteryVoltageMillivoltsEffective / 1000.0) + "V\n"
      " [+] Single voltage: " + String(__System.batteryVoltageMillivoltsEffective / (1000.0 * _LOWBATTERY_NUMBER_OF_BATTERIES)) + "V\n";

    //SD Card status
    statusMessage += "\nSD Card:\n";
    if(camera.sdOpen()) {
      statusMessage += " [+] Used space: " + String(camera.sdGetUsedSpace()) + "%\n"
        " [+] Number of photos: " + String(camera.sdGetPhotoCounter()) + "\n"
        " [+] Last photo date: " + ((camera.sdGetLastPhotoTimestamp() == 0) ? "-" : getDateFormat("%F, %T", camera.sdGetLastPhotoTimestamp())) + "\n";
    } else {
      statusMessage += " [-] not available\n";
    }
    camera.sdClose();

    //Send message
    telegram.sendMessage(statusMessage);
  }

  //Command: /blink
  //Blink the flash 5 times
  else if(strcmp("/blink", command) == 0) {
    for(uint8_t i=0; i<5; i++) {
      camera.flashBlink(10);
      delay(100);
    }
  }

  //Unknown command
  else {
    Serial.println(" [-] Unknown command");
  }
}