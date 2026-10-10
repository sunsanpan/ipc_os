/* GY-271 (QMC5883L or HMC5883L) driver — see gy271.h. */
#include "gy271.h"

#define QMC_ADDR     (0x0DU << 1)
#define QMC_REG_DATA 0x00U /* X L/H, Y L/H, Z L/H (little-endian) */
#define QMC_REG_CTL1 0x09U
#define QMC_REG_SRST 0x0BU
#define QMC_REG_ID   0x0DU /* reads 0xFF */

#define HMC_ADDR     (0x1EU << 1)
#define HMC_REG_CFGA 0x00U
#define HMC_REG_CFGB 0x01U
#define HMC_REG_MODE 0x02U
#define HMC_REG_DATA 0x03U /* X, Z, Y — big-endian, note the axis order */
#define HMC_REG_ID   0x0AU /* "H43" */

#define TIMEOUT_MS 100U

static bool rd(gy271_t *m, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(m->hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, buf, len, TIMEOUT_MS) == HAL_OK;
}

static bool wr(gy271_t *m, uint8_t addr, uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(m->hi2c, addr, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, TIMEOUT_MS) == HAL_OK;
}

bool gy271_init(gy271_t *m, I2C_HandleTypeDef *hi2c)
{
    uint8_t id[3] = {0};

    m->hi2c = hi2c;
    m->type = IPC_MAG_NONE;
    m->ok   = false;

    if (rd(m, QMC_ADDR, QMC_REG_ID, id, 1) && id[0] == 0xFFU)
    {
        /* SET/RESET period (datasheet mandates 0x01), then continuous mode,
         * 50 Hz ODR, 2 G range (12000 LSB/G), OSR 512 */
        if (!wr(m, QMC_ADDR, QMC_REG_SRST, 0x01U) || !wr(m, QMC_ADDR, QMC_REG_CTL1, 0x05U))
            return false;
        m->type = IPC_MAG_QMC5883L;
    }
    else if (rd(m, HMC_ADDR, HMC_REG_ID, id, 3) && id[0] == 'H' && id[1] == '4' && id[2] == '3')
    {
        /* 8-sample average, 15 Hz; gain 1090 LSB/G; continuous mode */
        if (!wr(m, HMC_ADDR, HMC_REG_CFGA, 0x70U) || !wr(m, HMC_ADDR, HMC_REG_CFGB, 0x20U) ||
            !wr(m, HMC_ADDR, HMC_REG_MODE, 0x00U))
            return false;
        m->type = IPC_MAG_HMC5883L;
    }
    else
    {
        return false;
    }

    m->ok = true;
    return true;
}

bool gy271_read(gy271_t *m, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t r[6];

    if (!m->ok)
        return false;

    if (m->type == IPC_MAG_QMC5883L && rd(m, QMC_ADDR, QMC_REG_DATA, r, 6))
    {
        *x = (int16_t)(r[1] << 8 | r[0]);
        *y = (int16_t)(r[3] << 8 | r[2]);
        *z = (int16_t)(r[5] << 8 | r[4]);
        return true;
    }
    if (m->type == IPC_MAG_HMC5883L && rd(m, HMC_ADDR, HMC_REG_DATA, r, 6))
    {
        *x = (int16_t)(r[0] << 8 | r[1]);
        *z = (int16_t)(r[2] << 8 | r[3]);
        *y = (int16_t)(r[4] << 8 | r[5]);
        return true;
    }

    m->ok = false;
    return false;
}
