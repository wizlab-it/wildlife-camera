/**
 * @package Wildlife Camera
 * Telegram support
 * @author WizLab.it
 * @version 20240308.087
 */

#include "telegram.h"


/**
 * Variables
 */
RTC_DATA_ATTR long __telegramLastUpdateId = -1;


/**
 * Telegram
 * Class constructor
 * @param apiToken    Telegram bot API token
 * @param chatId      Telegram Chat ID where to send messages
 * @param commandProcessorFunction    (optional) Pointer to the external function that process the received telegram commands
 */
Telegram::Telegram(String apiToken, int64_t chatId, void (*commandProcessorFunction)(const char*)) {
  _apiToken = apiToken;
  _chatId = chatId;
  _commandProcessorFunction = commandProcessorFunction;
}


/**
 * Telegram::getUpdates
 * Get updates on a telegram chat
 * @return    number of processed updates, negative value if error
 */
int8_t Telegram::getUpdates() {
  String payload = "offset=" + String(__telegramLastUpdateId);
  char* response;
  int8_t commStatus = _httpRequest(_TELEGRAM_COMMAND_GETUPDATES, (uint8_t*)(payload.c_str()), payload.length(), &response);
  int8_t updatesCount = 0;

  String logDatetime = getDateFormat("%F, %T");
  if(logDatetime == "") logDatetime = "Unknown date";
  Serial.printf("%s - Get telegram updates (from ID %ld): ", logDatetime.c_str(), __telegramLastUpdateId);
  if(commStatus == 0) {
    Serial.println("OK");

    //Parse response
    JsonDocument jsonParsed;
    deserializeJson(jsonParsed, response);

    //Process updates
    JsonArray updates = jsonParsed["result"].as<JsonArray>();
    if(updates.size() > 0) {
      for(JsonVariant update : updates) {
        long updateId = update["update_id"];
        const char* message = update["message"]["text"];

        //Set next Update ID
        __telegramLastUpdateId = updateId + 1;

        //Check if emssage is a command, then process it
        if(message[0] == '/') {
          strtok((char*)message, "@");
          Serial.printf(" [i] Received telegram command: %s\n", message);
          if(_commandProcessorFunction == NULL) {
            Serial.println(" [-] External command processor not defined");
          } else {
            _commandProcessorFunction(message);
          }
        }

        //Check max 10 updates per loop
        if(++updatesCount == 10) break;
      }
    }
  } else {
    Serial.printf("failed (err: %d)\n", commStatus);
  }

  //If commStatus is 0, then check was successful: returns number of processed updates
  if(commStatus == 0) return updatesCount;

  //If here, error during update: returns the error code
  return commStatus;
}


/**
 * Telegram::sendMessage
 * Send a message to a telegram chat
 * @param message   the message to send
 * @return          0 if successful, negative value if error
 */
int8_t Telegram::sendMessage(String message) {
  String payload = "chat_id=" + String(_chatId) + "&text=" + urlEncode(message);

  char* response;
  int8_t commStatus = _httpRequest(_TELEGRAM_COMMAND_MESSAGE, (uint8_t*)(payload.c_str()), payload.length(), &response);

  Serial.print(" [+] Send telegram message: ");
  if(commStatus == 0) {
    Serial.println("OK");
  } else {
    Serial.printf("failed (err: %d)\n", commStatus);
  }

  return commStatus;
}


/**
 * Telegram::sendAction
 * Send an action to a telegram chat
 * @param action    action to send (typing, upload_photo, record_video, upload_video, record_audio, upload_audio, upload_document, find_location)
 * @return          0 if successful, negative value if error
 */
int8_t Telegram::sendAction(String action) {
  String payload = "chat_id=" + String(_chatId) + "&action=" + action;
  char* response;
  int8_t commStatus = _httpRequest(_TELEGRAM_COMMAND_ACTION, (uint8_t*)(payload.c_str()), payload.length(), &response);
  return commStatus;
}


/**
 * Telegram::sendPhoto
 * Take a photo and send it to a telegram chat
 * @param photo         pointer to the photo data
 * @param photoLength   length of the photo data
 * @return              0 if successful, negative value if error
 */
int8_t Telegram::sendPhoto(uint8_t *photo, long photoLength) {
  //Check length
  if(photoLength < 1) return -1;

  //Send upload_photo action
  sendAction("upload_photo");

  //Prepare payload head and tail
  String caption = "Wildlife Camera photo on the " + getDateFormat("%F") + " at " + getDateFormat("%T") + "\r\nSD Used Space: " + String(cameraSdGetUsedSpace()) + "%";
  String payloadHead = "--" + String(_TELEGRAM_MULTIPART_BOUNDARY) + "\r\n"
    "Content-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + String(_chatId) + "\r\n--" + String(_TELEGRAM_MULTIPART_BOUNDARY) + "\r\n"
    "Content-Disposition: form-data; name=\"caption\"; \r\n\r\n" + caption + "\r\n--" + String(_TELEGRAM_MULTIPART_BOUNDARY) + "\r\n"
    "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String payloadTail = "\r\n--" + String(_TELEGRAM_MULTIPART_BOUNDARY) + "--\r\n";

  //Create payload
  long payloadLengthFinal = payloadHead.length() + photoLength + payloadTail.length(); //Calculate final payload length (head + image + tail)
  uint8_t *payload = (uint8_t *)malloc(payloadLengthFinal); //Allocate space for payload
  memcpy(payload, (uint8_t*)(payloadHead.c_str()), payloadHead.length()); //Add head to payload
  memcpy((payload + payloadHead.length()), photo, photoLength); //Add photo to payload
  memcpy((payload + payloadHead.length() + photoLength), (uint8_t*)(payloadTail.c_str()), payloadTail.length()); //Add tail to payload

  //Send request
  char* response;
  int8_t commStatus = _httpRequest(_TELEGRAM_COMMAND_PHOTO, payload, payloadLengthFinal, &response);
  Serial.print(" [+] Send telegram photo: ");
  if(commStatus == 0) {
    Serial.println("OK");
  } else {
    Serial.printf("failed (err: %d)\n", commStatus);
  }

  //Free payload and return
  free(payload);
  payload = NULL;
  return commStatus;
}


/**
 * Telegram::_httpRequest
 * Low-level communication with telegram server
 * @param command           telegram command (TELEGRAM_COMMAND_MESSAGE, TELEGRAM_COMMAND_PHOTO)
 * @param payload           data to be sent to telegram
 * @param payloadLength     data length
 * @param responseToReturn  pointer of a pointer that will be used to store the response
 * @return                  0 if successful, negative value if error
 */
int8_t Telegram::_httpRequest(uint8_t command, uint8_t* payload, long payloadLength, char** responseToReturn) {
  //Check if WiFi is connected
  if(WiFi.status() != WL_CONNECTED) return -101;

  //WiFi Client, no certificate check
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();

  //Set command params
  String endpoint;
  String contentType = "application/x-www-form-urlencoded";
  switch(command) {
    case _TELEGRAM_COMMAND_GETUPDATES:
      endpoint = "getUpdates";
      break;
    case _TELEGRAM_COMMAND_MESSAGE:
      endpoint = "sendMessage";
      break;
    case _TELEGRAM_COMMAND_ACTION:
      endpoint = "sendChatAction";
      break;
    case _TELEGRAM_COMMAND_PHOTO:
      endpoint = "sendPhoto";
      contentType = "multipart/form-data; boundary=" + String(_TELEGRAM_MULTIPART_BOUNDARY);
      break;
    default:
      return -102;
  }

  //Connect to telegram
  if(!wifiClient.connect(_TELEGRAM_HOSTNAME, 443)) return -103;

  //Send request header
  wifiClient.println("POST /bot" + String(_apiToken) + "/" + endpoint + " HTTP/1.1");
  wifiClient.println("Host: " + String(_TELEGRAM_HOSTNAME));
  wifiClient.println("Content-Length: " + String(payloadLength));
  wifiClient.println("Content-Type: " + contentType);
  wifiClient.println();

  //Send request body
  for(size_t n=0; n<payloadLength; n=n+1024) {
    if((n + 1024) < payloadLength) {
      wifiClient.write((payload + n), 1024);
    } else if((payloadLength % 1024) > 0) {
      wifiClient.write((payload + n), (payloadLength % 1024));
    }
  }

  //Get response
  uint8_t nlcrCounter = 0;
  bool isHeader = true;
  long waitUntil = millis() + (_TELEGRAM_WAIT_TIMEOUT * 1000);
  String response = "";

  while(waitUntil > millis()) {
    delay(100);
    while(wifiClient.available()) {
      char c = wifiClient.read();
      if(isHeader) {
        if((c == '\n') || (c == '\r')) nlcrCounter++;
        else nlcrCounter = 0;
        if(nlcrCounter == 4) isHeader = false;
      } else {
        response += String(c);
      }
    }
    if(response.length() != 0) break;
  }
  wifiClient.stop();
  if(response.length() == 0) return -104;

  //Prepare response to return
  *responseToReturn = (char *)malloc(response.length() + 5);
  response.toCharArray(*responseToReturn, response.length() + 1);

  //Process response
  JsonDocument json;
  deserializeJson(json, response);
  if(json["ok"] != true) return -105;

  //If here, all good
  return 0;
}