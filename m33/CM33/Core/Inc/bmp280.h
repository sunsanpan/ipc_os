/* BMP280 on STM32 HAL I2C. Bosch reference integer compensation. */
#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#define BMP280_I2C_ADDR (0x76U << 1) /* SDO low. Use 0x77 if SDO is tied high. */

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint16_t t1;
    int16_t  t2, t3;
    uint16_t p1;
    int16_t  p2, p3, p4, p5, p6, p7, p8, p9;
    int32_t  t_fine;
    bool     ok;
} bmp280_t;

bool bmp280_init(bmp280_t *d, I2C_HandleTypeDef *hi2c);
/* temp_cc = deg C * 100, press_pa = Pa. Clears d->ok on bus error. */
bool bmp280_read(bmp280_t *d, int32_t *temp_cc, uint32_t *press_pa);

#endif
