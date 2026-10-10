/*
 * A35 <-> M33 RPMsg wire protocol. Single copy, included by both sides
 * (m33/CM33 build adds the repo root to its include path; a35/Makefile
 * uses -I..). Plain C99, no HAL/Linux dependencies.
 *
 * Transport: OpenAMP VIRT_UART -> /dev/ttyRPMSG0 on Linux. Byte stream,
 * so every message is enveloped for resync:
 *
 *   +------+------+------+----------------+------+------+
 *   | 0xAA | TYPE | LEN  | PAYLOAD (LE)   | CRC  | 0x55 |
 *   +------+------+------+----------------+------+------+
 *
 * CRC = 8-bit XOR over bytes [0 .. 2+LEN]. Payload fields little-endian.
 */
#ifndef IPC_PROTOCOL_H
#define IPC_PROTOCOL_H

#include <stdint.h>

#define IPC_FRAME_SYNC     0xAAU
#define IPC_FRAME_FOOTER   0x55U
#define IPC_FRAME_OVERHEAD 5U
#define IPC_MAX_PAYLOAD    128U

/* M33 -> A35 */
#define IPC_TYPE_SENSOR 0x01U
#define IPC_TYPE_ACK    0x03U
/* A35 -> M33. The M33 endpoint cannot transmit until it has received one
 * frame from the host (OpenAMP learns the reply address from it), so the
 * A35 sends a PING right after opening the tty. */
#define IPC_TYPE_PING   0x10U

/* SENSOR payload, 20 bytes:
 *   u32 seq        sample counter
 *   i32 temp_cc    BMP280 temperature, deg C * 100
 *   u32 press_pa   BMP280 pressure, Pa
 *   i16 mag_x/y/z  GY-271 raw axis counts
 *   u8  flags      bit0 = BMP280 ok, bit1 = magnetometer ok
 *   u8  mag_type   IPC_MAG_*                                            */
#define IPC_SENSOR_LEN   20U
#define IPC_FLAG_BMP_OK  0x01U
#define IPC_FLAG_MAG_OK  0x02U
#define IPC_MAG_NONE     0U
#define IPC_MAG_QMC5883L 1U /* GY-271 (newer boards), 0x0D, 12000 LSB/G @ 2G */
#define IPC_MAG_HMC5883L 2U /* GY-271 (older boards), 0x1E, 1090 LSB/G default gain */

/* ACK payload, 2 bytes: u8 cmd type, u8 status (0 = ok) */
#define IPC_ACK_LEN 2U

static inline uint8_t ipc_xor_crc(const uint8_t *buf, uint16_t len)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; ++i)
        crc ^= buf[i];
    return crc;
}

/* Build a frame into out (must hold len + IPC_FRAME_OVERHEAD). Returns frame size. */
static inline uint16_t ipc_build_frame(uint8_t *out, uint8_t type, const uint8_t *payload, uint8_t len)
{
    out[0] = IPC_FRAME_SYNC;
    out[1] = type;
    out[2] = len;
    for (uint8_t i = 0; i < len; ++i)
        out[3 + i] = payload[i];
    out[3 + len] = ipc_xor_crc(out, (uint16_t)(3U + len));
    out[4 + len] = IPC_FRAME_FOOTER;
    return (uint16_t)(len + IPC_FRAME_OVERHEAD);
}

static inline void ipc_wr_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline void ipc_wr_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static inline uint32_t ipc_rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline int16_t ipc_rd_i16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

#endif /* IPC_PROTOCOL_H */
