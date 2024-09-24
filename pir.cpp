/**
 * @package Wildlife Camera
 * Motion sensor (PIR)
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240821.007
 */

#include "pir.h"


/**
 * Pir
 * Class constructor
 * @param pinSignal   Pin that receives the PIR signal
 * @param enabled     If true, PIR signal triggers photo shot
 */
Pir::Pir(bool enabled, gpio_num_t pinSignal) {
  //Save configurations
  _enabled = enabled;
  _pinSignal = pinSignal;

  //Signal pin is initially configured as INPUT to allow SD Card activation
  pinMode(_pinSignal, INPUT);
}


/**
 * Pir::enable
 * Enable PIR
 * @param isr     Pointer to the external PIR interrupt function
 */
void Pir::enable(void (*isr)()) {
  //Save interrupt function
  _isr = isr;

  //Signal pin changed to INPUT_PULLDOWN, then attach interrupt
  pinMode(_pinSignal, INPUT_PULLDOWN);
  attachInterrupt(_pinSignal, _isr, RISING);

  Serial.println(" [+] Motion sensor activated");
}


/**
 * Pir::prepareDeepSleep
 * Prepare PIR for deep sleep
 * @param enableWakeupByPir   If true, PIR can wake up device from deep sleep
 */
void Pir::prepareDeepSleep(bool enableWakeupByPir) {
  if(_enabled && enableWakeupByPir) {
    esp_sleep_enable_ext0_wakeup(_pinSignal, HIGH);
  }
}