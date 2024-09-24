/**
 * @package Wildlife Camera
 * Motion sensor (PIR)
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240821.004
 */

#ifndef PIR_H
#define PIR_H


/**
 * Includes
 */
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "extern.h"


/**
 * Class definition
 */
class Pir {
  private:
    gpio_num_t _pinSignal;
    bool _enabled;
    void (*_isr)();         //Pointer to external interrupt function

  public:
    Pir(bool enabled, gpio_num_t pinSignal);
    void enable(void (*isr)());
    void prepareDeepSleep(bool enableWakeupByPir);
};


#endif