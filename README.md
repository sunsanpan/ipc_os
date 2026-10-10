# ipc_os — M33 → A35 sensor link over RPMsg (STM32MP257F)

Bare-metal Cortex-M33 firmware reads a **BMP280** (temperature/pressure)
and a **GY-271** magnetometer (QMC5883L or HMC5883L, auto-detected) on
**I2C8**, and pushes the readings to the Cortex-A35 (Linux) over
**OpenAMP RPMsg**. A plain C program on the A35 prints them on the console.
No cFS.

```
   Cortex-M33 (bare metal)                    Cortex-A35 (OpenSTLinux)
┌───────────────────────────┐   RPMsg      ┌───────────────────────────┐
│ BMP280 @0x76  ┐ I2C8      │  VIRT_UART   │ /dev/ttyRPMSG0            │
│ GY-271 @0x0D/ ┘ PZ3 SDA   │─────────────►│ a35/rpmsg_sensor_rx       │
│        0x1E     PZ4 SCL   │◄── PING ─────│   → printf                │
│ 2 Hz sample loop          │              │                           │
└───────────────────────────┘              └───────────────────────────┘
```

Derived from the `sterg_rpmsg` / `m33_fw` pair in
[obc_fw_cfs (cfs-mpu)](https://github.com/STeRG-MIT-WPU/obc_fw_cfs/tree/cfs-mpu),
with the cFS app replaced by a standalone receiver.

## Layout

| Path | Content |
|---|---|
| `protocol.h` | Wire format, shared by both cores (single copy) |
| `m33/` | STM32CubeIDE project (`ipc_m33`). App code in `CM33/Core/Src/{sensor_app,bmp280,gy271}.c` |
| `a35/rpmsg_sensor_rx.c` | Linux receiver, `a35/Makefile`, `a35/start_m33.sh` |

## Wiring (STM32MP257F-DK, CN5)

| Signal | Pin | Note |
|---|---|---|
| SDA | PZ3 (CN5-3) | both sensors in parallel |
| SCL | PZ4 (CN5-5) | |
| VCC | 3V3 | |
| GND | GND | |

I2C8 and GPIOZ must be RIF-assigned to the M33 (the `ca35tdcid-ostl-m33-examples`
OP-TEE profile does this; see the reference repo's `docs/M33_RIF_BUILD_GUIDE.md`).

## Wire format

`[0xAA][TYPE][LEN][payload LE][XOR CRC over bytes 0..2+LEN][0x55]`

| TYPE | Dir | Payload |
|---|---|---|
| `0x01` SENSOR | M33→A35 | u32 seq, i32 temp °C×100, u32 press Pa, i16 mx, i16 my, i16 mz, u8 flags (b0 BMP ok, b1 mag ok), u8 mag type (1 QMC5883L, 2 HMC5883L) |
| `0x10` PING | A35→M33 | none. Sent once after open: the M33 endpoint cannot transmit until it has received one frame from the host |
| `0x03` ACK | M33→A35 | u8 cmd, u8 status |

## Build

**M33:** import `m33/` into STM32CubeIDE (File → Import → Existing Projects),
build the `ipc_m33_CM33` Debug configuration → `m33/CM33/Debug/ipc_m33_CM33.elf`.
Sign it for the OP-TEE remoteproc TA (mandatory on the ca35tdcid image):

```bash
python3 optee_os/scripts/sign_rproc_fw.py --in ipc_m33_CM33.elf --out ipc_m33_CM33.sig \
  --key optee_os/keys/default.pem --plat-tlv BOOTADDR 0x80100000
```

**A35:**

```bash
cd a35
make test                                   # host self-check of the frame parser
make CROSS_COMPILE=aarch64-linux-gnu-       # or run `make` natively on the board
```

## Run (on the board, as root)

```bash
cp ipc_m33_CM33.sig /lib/firmware/
./start_m33.sh                 # loads + starts the M33, /dev/ttyRPMSG0 appears
./rpmsg_sensor_rx              # prints one line per sample
```

```
link up: /dev/ttyRPMSG0
M33 ACK cmd=0x10 status=0
[     1] BMP280 T= 27.43 C  P=  950.12 hPa | QMC5883L X=  -512 Y=  1203 Z= -2210 (-0.043 0.100 -0.184 G)
[     2] BMP280 T= 27.44 C  P=  950.10 hPa | QMC5883L X=  -509 Y=  1199 Z= -2214 (-0.042 0.100 -0.185 G)
```

M33 boot log (sensor probe results, faults):
`cat /sys/kernel/debug/remoteproc/remoteproc1/trace0`.

A sensor that fails its probe shows as `---` and is re-probed every 5 s.
Sample period: `SENSOR_APP_PERIOD_MS` in `m33/CM33/Core/Inc/sensor_app.h`.
