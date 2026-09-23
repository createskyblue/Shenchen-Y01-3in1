**Language:** [English](README.md) | [中文](README.zh-CN.md)

# Shenchen Y01-3in1 · 3-in-1 Gas Sensor Module Driver

**Vendor:** Shenzhen Shenchen Technology Co., LTD  
**Model:** Y01-3in1  
**Type:** TVOC + CH₂O + CO₂ multi-gas, UART auto-upload  

Platform-independent C driver for the Shenchen **Y01-3in1** air-quality module.

![Y01-3in1 3-in-1 air quality module](img/PixPin_2026-09-24_03-57-22.jpg)

*Y01-3in1 multi-gas module (TVOC / CH₂O / CO₂) · Shenchen Technology*

![Y01 module front and back](img/PixPin_2026-09-24_03-57-28.jpg)

*Y01 module PCB front & back*

---

## Features

- Platform-independent: `sensor_io_t` injects `read` / `now_ms`
- Auto-upload module: call `get()` — no host commands required
- Outputs: `tvoc_mg_m3`, `ch2o_mg_m3`, `co2_ppm` (all `float`)
- 5-minute warm-up checked **after** a successful parse (`Y01_ERR_WARMUP`)
- Return codes: `0` OK, `1` no frame / bad args, `2` warming up

## IO interface you must implement

The driver never touches the HAL. It only uses function pointers you inject at `init` (see `src/sensor.h`):

```c
typedef struct sensor_io {
    /* Read UART: non-blocking; return bytes actually read; 0 = no data now */
    uint32_t (*read)(uint8_t *buf, uint32_t len);

    /* Write UART: module auto-uploads; driver never TX — may be NULL */
    uint32_t (*write)(const uint8_t *buf, uint32_t len);

    /* Millisecond tick: required; timeout & warm-up */
    uint32_t (*now_ms)(void);
} sensor_io_t;
```

Rules:

1. **`read` / `write` must be non-blocking** — no `delay` or wait-for-TX inside; return 0 when empty, the driver retries until its deadline.
2. **`now_ms` returns ms since boot** (wrap-around OK); warm-up and `wait_ms` use it.
3. On a shared UART, **flushing RX after channel switch is the caller’s job**.

The driver stays platform-independent: supply `io` at init; no MCU/RTOS assumptions.

> Note: the datasheet’s query command disables auto-upload until power-cycle — this driver never sends it.

## Usage

```c
#include "sensor_y01_3in1.h"

sensor_y01_t ctx;
sensor_y01_data_t data = {0};

sensor_y01_init(&ctx, &io);

uint32_t err = sensor_y01_get(&ctx, &data, Y01_WAIT_MS_TYPICAL);
if (err == Y01_OK) {
    /* data.tvoc_mg_m3 / ch2o_mg_m3 / co2_ppm */
} else if (err == Y01_ERR_WARMUP) {
    /* link OK, still within 5-minute warm-up */
} else {
    /* timeout / checksum / bad args */
}
```

Build: add `src/*.c` and put `src/` on the include path.

## Layout

| Path | Description |
|------|-------------|
| `src/sensor.h` | Dependency-injection (IO API) |
| `src/sensor_y01_3in1.h/.c` | Driver |
| `img/` | Product photos |
| `docs/三合一气体传感器模组_Y01-3in1.pdf` | Vendor manual (Chinese) |

---

**License:** MIT  
**Datasheet:** see `docs/`
