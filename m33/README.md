# ipc_m33 — Cortex-M33 firmware (CubeIDE project)

Samples BMP280 + GY-271 on I2C8 (PZ3/PZ4) and pushes framed readings to
the A35 over OpenAMP RPMsg (`/dev/ttyRPMSG0`). See `../README.md`.

| Path | Content |
|---|---|
| `ipc_m33.ioc` | CubeMX config (I2C8, RIF). USART6 is still configured but unused |
| `CM33/Core/Src/sensor_app.c` | sample loop, framing, PING/ACK |
| `CM33/Core/Src/bmp280.c`, `gy271.c` | sensor drivers |
| `../protocol.h` | wire format (repo root is on the include path: `../../..` from `CM33/Debug`) |
| `CM33/OPENAMP/`, `CM33/Middlewares/OpenAMP/` | OpenAMP glue, libmetal/open-amp/VIRT_UART (from STM32Cube_FW_MP2) |
| `CM33/Utilities/CoproSync/` | remoteproc shutdown handshake |
| `CM33/Core/Src/stm32mp2xx_hal_ipcc.c` | IPCC HAL kept under Core/ so CubeMX regen does not prune it |

Build with STM32CubeIDE ≥ 1.19 (CM33 configuration). If you regenerate
from CubeMX, re-check `.cproject` include paths (`../../..` must remain)
and the `HAL_IPCC_MODULE_ENABLED` define (`.extSettings` re-injects it).
