/*
 * A35 (Linux) side of the M33 sensor link. Opens /dev/ttyRPMSG0, sends the
 * wake-up PING, then prints every SENSOR frame the M33 pushes.
 *
 *   make            -> rpmsg_sensor_rx (set CROSS_COMPILE=aarch64-...- to cross build)
 *   make test       -> host self-check of the frame parser
 *   ./rpmsg_sensor_rx [/dev/ttyRPMSGn]
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "protocol.h"

/* ---- frame parser (pure, testable) -------------------------------------- */

typedef struct
{
    uint8_t  buf[IPC_MAX_PAYLOAD + IPC_FRAME_OVERHEAD];
    uint16_t n;        /* bytes of the current candidate frame collected */
    uint32_t errors;   /* resyncs after a bad CRC/footer/length */
} parser_t;

typedef void (*frame_cb)(uint8_t type, const uint8_t *payload, uint8_t len);

/* Feed one byte; calls cb for each complete valid frame. */
static void parser_feed(parser_t *p, uint8_t b, frame_cb cb)
{
    if (p->n == 0 && b != IPC_FRAME_SYNC)
        return;

    p->buf[p->n++] = b;

    if (p->n == 3 && p->buf[2] > IPC_MAX_PAYLOAD)
    {
        p->errors++;
        p->n = 0;
        return;
    }
    if (p->n < 3 || p->n < p->buf[2] + IPC_FRAME_OVERHEAD)
        return;

    uint8_t len = p->buf[2];
    if (ipc_xor_crc(p->buf, (uint16_t)(3 + len)) == p->buf[3 + len] && p->buf[4 + len] == IPC_FRAME_FOOTER)
        cb(p->buf[1], &p->buf[3], len);
    else
        p->errors++;
    p->n = 0;
}

/* ---- console output ----------------------------------------------------- */

#ifndef SELFTEST
static void print_frame(uint8_t type, const uint8_t *p, uint8_t len)
{
    if (type == IPC_TYPE_ACK && len == IPC_ACK_LEN)
    {
        printf("M33 ACK cmd=0x%02X status=%u\n", p[0], p[1]);
        return;
    }
    if (type != IPC_TYPE_SENSOR || len != IPC_SENSOR_LEN)
    {
        printf("?? type=0x%02X len=%u\n", type, len);
        return;
    }

    uint32_t seq   = ipc_rd_u32(&p[0]);
    int32_t  tcc   = (int32_t)ipc_rd_u32(&p[4]);
    uint32_t pa    = ipc_rd_u32(&p[8]);
    int16_t  x     = ipc_rd_i16(&p[12]);
    int16_t  y     = ipc_rd_i16(&p[14]);
    int16_t  z     = ipc_rd_i16(&p[16]);
    uint8_t  flags = p[18];
    uint8_t  mtype = p[19];

    printf("[%6u] ", seq);
    if (flags & IPC_FLAG_BMP_OK)
        printf("BMP280 T=%6.2f C  P=%8.2f hPa | ", tcc / 100.0, pa / 100.0);
    else
        printf("BMP280 --- | ");

    if (flags & IPC_FLAG_MAG_OK)
    {
        /* ponytail: nominal LSB/G from the config written in gy271.c; calibrate offsets/scale per board */
        double lsb_per_g = mtype == IPC_MAG_QMC5883L ? 12000.0 : 1090.0;
        printf("%s X=%6d Y=%6d Z=%6d (%.3f %.3f %.3f G)\n",
               mtype == IPC_MAG_QMC5883L ? "QMC5883L" : "HMC5883L", x, y, z,
               x / lsb_per_g, y / lsb_per_g, z / lsb_per_g);
    }
    else
        printf("MAG ---\n");
    fflush(stdout);
}
#endif

/* ---- self-test ---------------------------------------------------------- */
#ifdef SELFTEST
#include <assert.h>
static int got;
static void count_cb(uint8_t type, const uint8_t *p, uint8_t len)
{
    assert(type == IPC_TYPE_SENSOR && len == IPC_SENSOR_LEN && ipc_rd_u32(p) == 42);
    got++;
}
int main(void)
{
    uint8_t  payload[IPC_SENSOR_LEN] = {0}, frame[64], stream[200];
    parser_t ps = {0};
    ipc_wr_u32(payload, 42);
    uint16_t fl = ipc_build_frame(frame, IPC_TYPE_SENSOR, payload, sizeof payload);

    /* junk, a frame with a corrupt CRC, a frame with a bad LEN, then two good ones */
    size_t n = 0;
    stream[n++] = 0x11; stream[n++] = 0xAA; stream[n++] = 0x01; stream[n++] = 0xFF; /* bad LEN */
    memcpy(stream + n, frame, fl); stream[n + 5] ^= 1; n += fl;                     /* bad CRC */
    memcpy(stream + n, frame, fl); n += fl;
    memcpy(stream + n, frame, fl); n += fl;
    for (size_t i = 0; i < n; ++i)
        parser_feed(&ps, stream[i], count_cb);
    assert(got == 2 && ps.errors == 2);
    puts("selftest ok");
    return 0;
}
#else
/* ---- real main ---------------------------------------------------------- */
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>

static int open_raw(const char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return -1;
    struct termios t;
    if (tcgetattr(fd, &t) == 0)
    {
        cfmakeraw(&t); /* essential: a cooked tty would echo M33 frames back to it */
        t.c_cflag |= CREAD | CLOCAL;
        tcsetattr(fd, TCSANOW, &t);
    }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

int main(int argc, char **argv)
{
    const char *dev = argc > 1 ? argv[1] : "/dev/ttyRPMSG0";
    parser_t    ps  = {0};
    uint8_t     ping[IPC_FRAME_OVERHEAD], buf[512];

    setvbuf(stdout, NULL, _IOLBF, 0);

    for (;;)
    {
        int fd = open_raw(dev);
        if (fd < 0)
        {
            fprintf(stderr, "open %s: %s (is the M33 firmware started?) retrying in 2 s\n", dev, strerror(errno));
            sleep(2);
            continue;
        }
        printf("link up: %s\n", dev);

        ipc_build_frame(ping, IPC_TYPE_PING, NULL, 0);
        if (write(fd, ping, sizeof ping) != (ssize_t)sizeof ping)
            fprintf(stderr, "wake-up PING failed: %s\n", strerror(errno));

        for (;;)
        {
            struct pollfd pfd = {.fd = fd, .events = POLLIN};
            int           pr  = poll(&pfd, 1, 5000);
            if (pr == 0)
            {
                fprintf(stderr, "no data for 5 s\n");
                continue;
            }
            if (pr < 0 && errno == EINTR)
                continue;
            ssize_t n = pr > 0 ? read(fd, buf, sizeof buf) : -1;
            if (n <= 0)
            {
                if (n < 0 && (errno == EINTR || errno == EAGAIN))
                    continue;
                break; /* hangup: M33 stopped/rebooted, reopen */
            }
            for (ssize_t i = 0; i < n; ++i)
                parser_feed(&ps, buf[i], print_frame);
        }

        close(fd);
        fprintf(stderr, "link down, reopening\n");
        ps.n = 0;
    }
}
#endif
