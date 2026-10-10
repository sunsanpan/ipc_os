/*
 * M33 sensor application: samples BMP280 + GY-271 on I2C8 and pushes
 * framed readings to the A35 over the OpenAMP VIRT_UART (/dev/ttyRPMSG0).
 *
 *   SensorApp_Init(&hi2c8);           // after MX_I2C8_Init + MX_OPENAMP_Init
 *   while (1) { OPENAMP_check_for_message(); SensorApp_Loop(); }
 */
#ifndef SENSOR_APP_H
#define SENSOR_APP_H

#include "main.h"

#define SENSOR_APP_PERIOD_MS  500U  /* sample + push rate */
#define SENSOR_APP_REPROBE_MS 5000U /* backoff before re-probing a dead sensor */

void SensorApp_Init(I2C_HandleTypeDef *hi2c);
void SensorApp_Loop(void);
void SensorApp_DeInit(void);

#endif
