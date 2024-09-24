/**
 * @package Wildlife Camera
 * Telegram support header
 * @author WizLab.it
 * @version 20240708.032
 */

#ifndef TELEGRAM_H
#define TELEGRAM_H


/**
 * Defines
 */
#define _TELEGRAM_HOSTNAME             "api.telegram.org"
#define _TELEGRAM_MULTIPART_BOUNDARY   "TelegramMultipartBoundary"
#define _TELEGRAM_WAIT_TIMEOUT         10

#define _TELEGRAM_COMMAND_GETUPDATES   1
#define _TELEGRAM_COMMAND_MESSAGE      2
#define _TELEGRAM_COMMAND_PHOTO        3
#define _TELEGRAM_COMMAND_ACTION       4


/**
 * Includes
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UrlEncode.h>
#include "extern.h"


/**
 * Class definition
 */
class Telegram {
  private:
    String _apiToken;
    int64_t _chatId;
    void (*_commandProcessorFunction)(const char*); //Pointer to external command processor function

    int8_t _httpRequest(uint8_t command, uint8_t* payload, long payloadLength, char** responseToReturn);

  public:
    Telegram(String apiToken, int64_t chatId, void (*commandProcessorFunction)(const char*) = NULL);
    int8_t getUpdates();
    int8_t sendMessage(String message);
    int8_t sendPhoto(uint8_t *photo, long photoLength);
    int8_t sendAction(String action);
};


#endif