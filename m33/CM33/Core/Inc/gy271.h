/*
 * GY-271 magnetometer breakout. The board ships with either a QMC5883L
 * (I2C 0x0D, most boards sold today) or the original HMC5883L (0x1E);
 * both are probed and the one that answers is used.
 */
#ifndef GY271_H
#define GY271_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "protocol.h" /* IPC_MAG_* type codes */

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t            type; /* IPC_MAG_NONE / QMC5883L / HMC5883L */
    bool               ok;
} gy271_t;

bool gy271_init(gy271_t *m, I2C_HandleTypeDef *hi2c);
/* Raw axis counts. Clears m->ok on bus error. */
bool gy271_read(gy271_t *m, int16_t *x, int16_t *y, int16_t *z);

#endif
