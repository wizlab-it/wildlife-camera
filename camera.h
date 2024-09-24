/**
 * @package Wildlife Camera
 * Camera handling header
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240228.018
 */

#ifndef CAMERA_H
#define CAMERA_H


/**
 * Includes
 */
#include <Arduino.h>
#include <CRC32.h>
#include "FS.h"
#include "SD_MMC.h"
#include "esp_camera.h"
#include "extern.h"


/**
 * Defines
 */
//Camera Parameters (AI-Thinker ESP32-CAM)
#define _CAMERA_PWDN_GPIO_NUM     32
#define _CAMERA_RESET_GPIO_NUM    -1
#define _CAMERA_XCLK_GPIO_NUM      0
#define _CAMERA_SIOD_GPIO_NUM     26
#define _CAMERA_SIOC_GPIO_NUM     27
#define _CAMERA_Y9_GPIO_NUM       35
#define _CAMERA_Y8_GPIO_NUM       34
#define _CAMERA_Y7_GPIO_NUM       39
#define _CAMERA_Y6_GPIO_NUM       36
#define _CAMERA_Y5_GPIO_NUM       21
#define _CAMERA_Y4_GPIO_NUM       19
#define _CAMERA_Y3_GPIO_NUM       18
#define _CAMERA_Y2_GPIO_NUM        5
#define _CAMERA_VSYNC_GPIO_NUM    25
#define _CAMERA_HREF_GPIO_NUM     23
#define _CAMERA_PCLK_GPIO_NUM     22

//SD and Photo DB params
#define _CAMERA_SD_BASE_PATH  "/WildlifeCameraPics"
#define _CAMERA_PHOTODB _CAMERA_SD_BASE_PATH "/photoDB.dat"
#define _CAMERA_PHOTODB_FILENAME_MAX_LENGTH 100

//Flash PIN
#define _CAMERA_FLASH_PIN         GPIO_NUM_4


/**
 * Class definition
 */
class Camera {
  private:
    struct _PhotoDB {
      uint16_t photoCounter = 0;
      char lastPhotoFilename[_CAMERA_PHOTODB_FILENAME_MAX_LENGTH];
      unsigned long lastPhotoTimestamp = 0;
    };

    struct _PhotoDBPackage {
      _PhotoDB photoDB;
      uint32_t crc;
    };

    struct _PhotoDBPackage photoDBPack;
    bool _sdIsOpen;
    framesize_t _frameSize;
    int _jpegQuality;
    bool _sdCardEnabled;

    void _sdPhotoDbSave();

  public:
    Camera(framesize_t frameSize, int jpegQuality, bool sdCardEnabled);
    bool init();
    long takePhoto(uint8_t **image, bool useFlash);
    void flashBlink(uint16_t duration);
    void flashGpioHold(bool status);
    bool sdOpen();
    void sdClose();
    float sdGetUsedSpace();
    uint16_t sdGetPhotoCounter();
    unsigned long sdGetLastPhotoTimestamp();
};


#endif