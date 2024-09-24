/**
 * @package Wildlife Camera
 * External functions
 * @author WizLab.it
 * @board AI-Thinker ESP32-CAM
 * @version 20240301.005
 */

#ifndef EXTERN_H
#define EXTERN_H


/**
 * External functions
 */
extern unsigned long setWakeupEnd(unsigned long increase);
extern unsigned long getTimestamp();
extern unsigned long getUptime();
extern String getDateFormat(String format, time_t timestamp = 0);
extern uint32_t getBatteryVoltage(bool getRaw = true);
extern uint8_t getBatteryLevel();
extern float cameraSdGetUsedSpace();


#endif