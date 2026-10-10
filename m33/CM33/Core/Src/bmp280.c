/* BMP280 driver — register map and compensation per Bosch datasheet. */
#include "bmp280.h"

#define REG_CALIB     0x88U
#define REG_CHIP_ID   0xD0U
#define REG_CTRL_MEAS 0xF4U
#define REG_CONFIG    0xF5U
#define REG_PRESS_MSB 0xF7U
#define TIMEOUT_MS    100U

static bool rd(bmp280_t *d, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(d->hi2c, BMP280_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, TIMEOUT_MS) == HAL_OK;
}

static bool wr(bmp280_t *d, uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(d->hi2c, BMP280_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, TIMEOUT_MS) == HAL_OK;
}

bool bmp280_init(bmp280_t *d, I2C_HandleTypeDef *hi2c)
{
    uint8_t id = 0, c[24];

    d->hi2c = hi2c;
    d->ok   = false;

    if (!rd(d, REG_CHIP_ID, &id, 1) || (id != 0x58U && id != 0x60U)) /* 0x58 BMP280, 0x60 BME280 */
        return false;
    /* normal mode, T x2, P x16; standby 1000 ms, filter x16 */
    if (!wr(d, REG_CTRL_MEAS, 0x57U) || !wr(d, REG_CONFIG, 0xA0U) || !rd(d, REG_CALIB, c, sizeof c))
        return false;

    d->t1 = (uint16_t)(c[1] << 8 | c[0]);
    d->t2 = (int16_t)(c[3] << 8 | c[2]);
    d->t3 = (int16_t)(c[5] << 8 | c[4]);
    d->p1 = (uint16_t)(c[7] << 8 | c[6]);
    d->p2 = (int16_t)(c[9] << 8 | c[8]);
    d->p3 = (int16_t)(c[11] << 8 | c[10]);
    d->p4 = (int16_t)(c[13] << 8 | c[12]);
    d->p5 = (int16_t)(c[15] << 8 | c[14]);
    d->p6 = (int16_t)(c[17] << 8 | c[16]);
    d->p7 = (int16_t)(c[19] << 8 | c[18]);
    d->p8 = (int16_t)(c[21] << 8 | c[20]);
    d->p9 = (int16_t)(c[23] << 8 | c[22]);
    d->ok = true;
    return true;
}

bool bmp280_read(bmp280_t *d, int32_t *temp_cc, uint32_t *press_pa)
{
    uint8_t raw[6];

    if (!d->ok || !rd(d, REG_PRESS_MSB, raw, sizeof raw))
    {
        d->ok = false;
        return false;
    }

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);

    /* temperature */
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)d->t1 << 1))) * (int32_t)d->t2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)d->t1) * ((adc_T >> 4) - (int32_t)d->t1)) >> 12) * (int32_t)d->t3) >> 14;
    d->t_fine  = v1 + v2;
    *temp_cc   = (d->t_fine * 5 + 128) >> 8;

    /* pressure, 64-bit variant -> Q24.8 Pa */
    int64_t a = (int64_t)d->t_fine - 128000;
    int64_t b = a * a * (int64_t)d->p6;
    b += (a * (int64_t)d->p5) << 17;
    b += (int64_t)d->p4 << 35;
    a = ((a * a * (int64_t)d->p3) >> 8) + ((a * (int64_t)d->p2) << 12);
    a = ((((int64_t)1 << 47) + a) * (int64_t)d->p1) >> 33;
    if (a == 0)
    {
        *press_pa = 0;
        return true;
    }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - b) * 3125) / a;
    a = ((int64_t)d->p9 * (p >> 13) * (p >> 13)) >> 25;
    b = ((int64_t)d->p8 * p) >> 19;
    p = ((p + a + b) >> 8) + ((int64_t)d->p7 << 4);
    *press_pa = (uint32_t)(p >> 8);
    return true;
}
