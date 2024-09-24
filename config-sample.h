/**
 * @package Wildlife Camera
 * Configuration
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240823.059
 */

#ifndef CONFIG_H
#define CONFIG_H


//Low battery
#define _LOWBATTERY_PIN                  GPIO_NUM_13
#define _LOWBATTERY_NUMBER_OF_BATTERIES  2            //Number of 18650 3.7V batteries
#define _LOWBATTERY_VDIV_RATIO           3.04         //Battery voltage divider ratio
#define _LOWBATTERY_CACHE_TIMEOUT        900          //In seconds, how long the battery level should be cached
#define _LOWBATTERY_CRITICAL_DEEP_SLEEP  10           //Hours to sleep if battery is critically low


//PIR (motion detector)
#define PIR_PIN       GPIO_NUM_12   //Pin that receives PIR signal
#define PIR_ENABLED   true          //Use PIR for motion detection


//Camera and SD Card
#define CAMERA_FRAME_SIZE       FRAMESIZE_UXGA  //see framesize_t
#define CAMERA_QUALITY          8               //JPG quality (1-100, low value is better quality)
#define CAMERA_SDCARD_ENABLED   true            //Use SD Card to save photos


//NTP
#define NTP_SERVER      "pool.ntp.org"    //NTP Server
#define NTP_TIMEZONE    +1                //Timezone


//WIFI
#define WIFI_SSID   "wifi-ssid"
#define WIFI_PSK    "psk"


//Telegram
#define TELEGRAM_BOT_NAME       "telegram-bot-name"
#define TELEGRAM_BOT_USERNAME   "telegram-bot-username"
#define TELEGRAM_BOT_API_TOKEN  "telegram-bot-api-token"
#define TELEGRAM_CHAT_ID        -12345


#endif