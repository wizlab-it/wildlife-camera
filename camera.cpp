/**
 * @package Wildlife Camera
 * Camera handling
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240811.044
 */

#include "camera.h"


/**
 * Camera
 * Class constructor
 * @param frameSize       Size of the photo (see framesize_t)
 * @param jpegQuality     JPG quality (1-100, low value is better quality)
 * @param sdCardEnabled   Use SD Card to save photos
 */
Camera::Camera(framesize_t frameSize, int jpegQuality, bool sdCardEnabled) {
  _frameSize = frameSize;
  _jpegQuality = jpegQuality;
  _sdCardEnabled = sdCardEnabled;
  _sdIsOpen = false;
}


/**
 * Camera::init
 * Camera Configuration
 * @return    true if camera is successfully initialized; false otherwise
 */
bool Camera::init() {
  //Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = _CAMERA_Y2_GPIO_NUM;
  config.pin_d1 = _CAMERA_Y3_GPIO_NUM;
  config.pin_d2 = _CAMERA_Y4_GPIO_NUM;
  config.pin_d3 = _CAMERA_Y5_GPIO_NUM;
  config.pin_d4 = _CAMERA_Y6_GPIO_NUM;
  config.pin_d5 = _CAMERA_Y7_GPIO_NUM;
  config.pin_d6 = _CAMERA_Y8_GPIO_NUM;
  config.pin_d7 = _CAMERA_Y9_GPIO_NUM;
  config.pin_xclk = _CAMERA_XCLK_GPIO_NUM;
  config.pin_pclk = _CAMERA_PCLK_GPIO_NUM;
  config.pin_vsync = _CAMERA_VSYNC_GPIO_NUM;
  config.pin_href = _CAMERA_HREF_GPIO_NUM;
  config.pin_sscb_sda = _CAMERA_SIOD_GPIO_NUM;
  config.pin_sscb_scl = _CAMERA_SIOC_GPIO_NUM;
  config.pin_pwdn = _CAMERA_PWDN_GPIO_NUM;
  config.pin_reset = _CAMERA_RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.frame_size = _frameSize;
  config.jpeg_quality = _jpegQuality;
  config.fb_count = 1;

  //Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if(err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }

  //Flash
  pinMode(_CAMERA_FLASH_PIN, OUTPUT);
  digitalWrite(_CAMERA_FLASH_PIN, LOW);

  //If here, all good
  return true;
}


/**
 * Camera::takePhoto
 * Takes a photo, optionally using the built-in flash
 * @param image     pointer of a pointer that will be used to store the image data (original variable should be NULL)
 * @param useFlash  if true, the flash is activated
 * @return          size of the *image* memory block, or negative value in the case of failure (-1: photo capture failed; -2: photo size is 0)
 */
long Camera::takePhoto(uint8_t **image, bool useFlash) {
  //Check if to activate flash
  if(useFlash == true) {
    digitalWrite(_CAMERA_FLASH_PIN, HIGH);
    delay(50);
  }

  //Dispose first picture because of bad quality
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);

  //Takes a new photo
  fb = NULL;
  fb = esp_camera_fb_get();

  //Deactivate flash
  digitalWrite(_CAMERA_FLASH_PIN, LOW);

  //Check if photo capture failed
  if(!fb) return -1;
  if(fb->len == 0) return -2;

  //Save photo on SD Card
  if(sdOpen()) {

    //Build path and file name based on datetime info
    String path = _CAMERA_SD_BASE_PATH;
    String filename = "/WCP-";
    if(getDateFormat("%Y") == "") {
      //No datetime info
      path += "/UnknownDate";
      filename += String(random(100000000, 999999999)) + ".jpg";
    } else {
      //Datetime info available
      path += "/" + getDateFormat("%F");
      filename += getDateFormat("%Y%m%d-%H%M%S") + ".jpg";
    }
    String pathfilename = path + filename;

    //Save photo on SD Card
    if(SD_MMC.mkdir(path.c_str())) {
      fs::FS &fs = SD_MMC; 
      File file = fs.open(pathfilename.c_str(), FILE_WRITE);
      if(file) {
        size_t wb = file.write(fb->buf, fb->len);
        Serial.printf(" [+] Photo saved on SD Card: %s (%d bytes)\n", pathfilename.c_str(), wb);
      }
      file.close();

      //Update Photo DB
      photoDBPack.photoDB.photoCounter++;
      memset(photoDBPack.photoDB.lastPhotoFilename, 0x00, _CAMERA_PHOTODB_FILENAME_MAX_LENGTH);
      strncpy(photoDBPack.photoDB.lastPhotoFilename, pathfilename.c_str(), (_CAMERA_PHOTODB_FILENAME_MAX_LENGTH - 1));
      photoDBPack.photoDB.lastPhotoTimestamp = getTimestamp();
      _sdPhotoDbSave();
    }
  }
  sdClose();

  //Copy image data in memory space
  long imageSize = fb->len;
  *image = (uint8_t *)malloc(imageSize);
  memcpy(*image, fb->buf, imageSize);
  esp_camera_fb_return(fb);

  return imageSize;
}


/**
 * Camera::flashBlink
 * Blink the flash
 * @param duration    Blink duration in milliseconds
 */
void Camera::flashBlink(uint16_t duration) {
  digitalWrite(_CAMERA_FLASH_PIN, 1);
  delay(duration);
  digitalWrite(_CAMERA_FLASH_PIN, 0);
}


/**
 * Camera::flashGpioHold
 * Set hold status for flash GPIO pin
 * @param status    true: enable hold; false: disable hold
 */
void Camera::flashGpioHold(bool status) {
  if(status) {
    gpio_hold_en(_CAMERA_FLASH_PIN);
  } else {
    gpio_hold_dis(_CAMERA_FLASH_PIN);
  }
}


/**
 * Camera::sdOpen
 * Open SD Card, check if base path exists
 * @return    true if SD Card is successfully opened; false otherwise
 */
bool Camera::sdOpen() {
  //Check if to use SD Card
  if(!_sdCardEnabled) return false;

  //Check if SD is already open
  if(_sdIsOpen) return true;

  //Try to open SD Card and base path
  if(!SD_MMC.begin("/sdcard", true) || (SD_MMC.cardType() == CARD_NONE) || !SD_MMC.mkdir(_CAMERA_SD_BASE_PATH)) {
    Serial.println(" [-] Error opening SD Card");
    sdClose();
    return false;
  }

  //Check system Photo DB CRC: if not valid, then try to load Photo DB from SD Card
  uint32_t crc = CRC32::calculate((const uint8_t *)&photoDBPack.photoDB, sizeof(_PhotoDB));
  if(photoDBPack.crc != crc) {

    //Try to load Photo DB from SD Card
    if(SD_MMC.exists(_CAMERA_PHOTODB)) {
      fs::FS &fs = SD_MMC;
      uint32_t crcTmp;

      //Read Photo DB on SD Card
      struct _PhotoDBPackage photoDBTmp;
      File file = fs.open(_CAMERA_PHOTODB, FILE_READ);
      if(file) file.read((uint8_t *)&photoDBTmp, sizeof(_PhotoDBPackage));
      file.close();

      //Check CRC of Photo DB from SD Card and if last photo file exists: if OK, then load it in system Photo DB
      crcTmp = CRC32::calculate((const uint8_t *)&photoDBTmp.photoDB, sizeof(_PhotoDB));
      if((photoDBTmp.crc == crcTmp) && SD_MMC.exists(photoDBTmp.photoDB.lastPhotoFilename)) {
        memcpy(&photoDBPack, &photoDBTmp, sizeof(_PhotoDBPackage));
      } else {
        //Photo DB on SD Card is invalid, delete it
        Serial.println(" [-] Removed invalid Photo DB on SD Card");
        SD_MMC.remove(_CAMERA_PHOTODB);
      }
    }
  }

  _sdIsOpen = true;
  return true;
}


/**
 * Camera::sdClose
 * Close SD Card
 */
void Camera::sdClose() {
  _sdIsOpen = false;
  SD_MMC.end();

  //Free pins 12 and 13
  pinMode(12, INPUT);
  pinMode(13, INPUT);
}


/**
 * Camera::sdGetUsedSpace
 * Get percentage of used space on SD Card
 * @return    SD card usage percentage
 */
float Camera::sdGetUsedSpace() {
  float usedSpace = -1.0;
  if(sdOpen()) {
    usedSpace = SD_MMC.usedBytes() * 100.00 / SD_MMC.totalBytes();
  }
  sdClose();
  return usedSpace;
}


/**
 * Camera::sdGetPhotoCounter
 * Get number of photo on SD Card
 * @return    Number of photo in Photo DB
 */
uint16_t Camera::sdGetPhotoCounter() {
  return photoDBPack.photoDB.photoCounter;
}


/**
 * Camera::sdGetLastPhotoTimestamp
 * Get timestamp of the last photo
 * @return    Timestamp of the last photo
 */
unsigned long Camera::sdGetLastPhotoTimestamp() {
  return photoDBPack.photoDB.lastPhotoTimestamp;
}


/**
 * Camera::_photoDbSave
 * Save system Photo DB on SD Card
 * Also check the used space on SD Card
 */
void Camera::_sdPhotoDbSave() {
  //Calculate Photo DB  CRC
  photoDBPack.crc = CRC32::calculate((const uint8_t *)&photoDBPack.photoDB, sizeof(_PhotoDB));

  //Save on SD Card
  fs::FS &fs = SD_MMC;
  File file = fs.open(_CAMERA_PHOTODB, FILE_WRITE);
  if(file) file.write((uint8_t *)&photoDBPack, sizeof(_PhotoDBPackage));
  file.close();
}