/* M33 sensor application — see sensor_app.h. */
#include "sensor_app.h"
#include "protocol.h"
#include "bmp280.h"
#include "gy271.h"

#include "openamp.h"
#include "virt_uart.h"

#include <stdio.h>

static VIRT_UART_HandleTypeDef huart0; /* -> /dev/ttyRPMSG0 */
static I2C_HandleTypeDef      *i2c;
static bmp280_t                bmp;
static gy271_t                 mag;

static uint32_t seq;
static uint32_t next_sample;
static uint32_t next_reprobe;

/* RX ring filled by the VIRT_UART callback (runs inside OPENAMP_check_for_message),
 * drained by the loop. The host only ever sends tiny PING frames. */
static volatile uint8_t  rx_ring[64];
static volatile uint8_t  rx_head;
static uint8_t           rx_tail;

static void send_frame(uint8_t type, const uint8_t *payload, uint8_t len)
{
    uint8_t  f[IPC_MAX_PAYLOAD + IPC_FRAME_OVERHEAD];
    uint16_t n = ipc_build_frame(f, type, payload, len);
    VIRT_UART_Transmit(&huart0, f, n);
}

static void rx_cb(VIRT_UART_HandleTypeDef *h)
{
    for (uint16_t i = 0; i < h->RxXferSize; ++i)
    {
        uint8_t next = (uint8_t)((rx_head + 1U) % sizeof rx_ring);
        if (next == rx_tail)
            break;
        rx_ring[rx_head] = h->pRxBuffPtr[i];
        rx_head          = next;
    }
}

/* Any complete, valid frame from the host is answered with an ACK. The
 * only command is PING; its real job is teaching OpenAMP our reply address. */
static void process_rx(void)
{
    while (rx_head != rx_tail)
    {
        uint8_t avail = (uint8_t)((rx_head + sizeof rx_ring - rx_tail) % sizeof rx_ring);
        uint8_t b0    = rx_ring[rx_tail];

        if (b0 != IPC_FRAME_SYNC)
        {
            rx_tail = (uint8_t)((rx_tail + 1U) % sizeof rx_ring);
            continue;
        }
        if (avail < IPC_FRAME_OVERHEAD)
            return;

        uint8_t len = rx_ring[(rx_tail + 2U) % sizeof rx_ring];
        if (len > sizeof rx_ring - IPC_FRAME_OVERHEAD)
        {
            rx_tail = (uint8_t)((rx_tail + 1U) % sizeof rx_ring);
            continue;
        }
        if (avail < len + IPC_FRAME_OVERHEAD)
            return;

        uint8_t f[sizeof rx_ring];
        for (uint8_t i = 0; i < len + IPC_FRAME_OVERHEAD; ++i)
            f[i] = rx_ring[(rx_tail + i) % sizeof rx_ring];

        if (ipc_xor_crc(f, (uint16_t)(3U + len)) == f[3U + len] && f[4U + len] == IPC_FRAME_FOOTER)
        {
            uint8_t ack[IPC_ACK_LEN] = {f[1], (uint8_t)(f[1] == IPC_TYPE_PING ? 0U : 0xFFU)};
            send_frame(IPC_TYPE_ACK, ack, sizeof ack);
            rx_tail = (uint8_t)((rx_tail + len + IPC_FRAME_OVERHEAD) % sizeof rx_ring);
        }
        else
        {
            rx_tail = (uint8_t)((rx_tail + 1U) % sizeof rx_ring);
        }
    }
}

static void sample_and_send(void)
{
    uint8_t  p[IPC_SENSOR_LEN] = {0};
    int32_t  t  = 0;
    uint32_t pr = 0;
    int16_t  x = 0, y = 0, z = 0;
    uint8_t  flags = 0;

    if (bmp280_read(&bmp, &t, &pr))
        flags |= IPC_FLAG_BMP_OK;
    if (gy271_read(&mag, &x, &y, &z))
        flags |= IPC_FLAG_MAG_OK;

    ipc_wr_u32(&p[0], ++seq);
    ipc_wr_u32(&p[4], (uint32_t)t);
    ipc_wr_u32(&p[8], pr);
    ipc_wr_u16(&p[12], (uint16_t)x);
    ipc_wr_u16(&p[14], (uint16_t)y);
    ipc_wr_u16(&p[16], (uint16_t)z);
    p[18] = flags;
    p[19] = mag.type;

    send_frame(IPC_TYPE_SENSOR, p, sizeof p);
}

void SensorApp_Init(I2C_HandleTypeDef *hi2c)
{
    i2c = hi2c;

    if (VIRT_UART_Init(&huart0) != VIRT_UART_OK)
        Error_Handler();
    VIRT_UART_RegisterCallback(&huart0, VIRT_UART_RXCPLT_CB_ID, rx_cb);

    /* A missing sensor is reported, never fatal: the loop keeps pushing
     * frames with the ok-flag clear and re-probes every SENSOR_APP_REPROBE_MS. */
    printf("M33: BMP280 %s\r\n", bmp280_init(&bmp, hi2c) ? "ok" : "not connected");
    printf("M33: GY-271 %s (type %u)\r\n", gy271_init(&mag, hi2c) ? "ok" : "not connected", mag.type);

    next_sample  = HAL_GetTick() + SENSOR_APP_PERIOD_MS;
    next_reprobe = HAL_GetTick() + SENSOR_APP_REPROBE_MS;
}

void SensorApp_DeInit(void)
{
    VIRT_UART_DeInit(&huart0);
}

void SensorApp_Loop(void)
{
    uint32_t now = HAL_GetTick();

    process_rx();

    if ((int32_t)(now - next_sample) >= 0)
    {
        next_sample += SENSOR_APP_PERIOD_MS;
        sample_and_send();
    }

    /* Re-probe faulted sensors with a backoff instead of hammering the bus. */
    if ((!bmp.ok || !mag.ok) && (int32_t)(now - next_reprobe) >= 0)
    {
        next_reprobe = now + SENSOR_APP_REPROBE_MS;
        if (!bmp.ok)
            bmp280_init(&bmp, i2c);
        if (!mag.ok)
            gy271_init(&mag, i2c);
    }
}
