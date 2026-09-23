/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor_y01_3in1.h
 * @brief   Platform-independent driver for Shenchen Y01-3in1 multi-gas module
 *
 * Vendor:  Shenzhen Shenchen Technology Co., LTD
 * Model:   Y01-3in1 (TVOC / CH2O / CO2)
 * Link:    UART 9600/8N1, factory auto-upload, fixed 9-byte frames
 *
 * Upload-only: this driver never transmits (query mode would disable
 * auto-upload until power-cycle). IO via sensor_io_t; no platform headers.
 ******************************************************************************
 */

#ifndef SENSOR_Y01_3IN1_H_
#define SENSOR_Y01_3IN1_H_

#include "sensor.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol constants */
#define Y01_FRAME_LEN       9u      /*!< Frame length including address and checksum */
#define Y01_ADDR_HI         0x2Cu   /*!< Fixed address byte 0 */
#define Y01_ADDR_LO         0xE4u   /*!< Fixed address byte 1 */

/*! Module uploads about once per second */
#ifndef Y01_WAIT_MS_TYPICAL
#define Y01_WAIT_MS_TYPICAL 1500u
#endif

/*! Warm-up after init: 5 minutes (checked only after a successful parse) */
#ifndef Y01_WARMUP_MS
#define Y01_WARMUP_MS       300000u
#endif

/* get() results: parse first, then warm-up — avoids mixing with I/O errors */
#define Y01_OK              0u  /*!< Parsed OK and warm-up done; out written */
#define Y01_ERR_IO          1u  /*!< Bad args / no frame; out untouched */
#define Y01_ERR_WARMUP      2u  /*!< Frame OK but warm-up not finished; out untouched */

/*! Parsed sample */
typedef struct {
    uint8_t valid;          /*!< 1 = at least one good frame received */
    float   tvoc_mg_m3;     /*!< TVOC, mg/m³, range 0–2.000 */
    float   ch2o_mg_m3;     /*!< Formaldehyde, mg/m³, range 0–1.000 */
    float   co2_ppm;        /*!< CO2, ppm, range 350–2000 (derived) */
} sensor_y01_data_t;

/*!
 * Driver context. No internal sample cache. Warm-up is judged after
 * a successful parse.
 */
typedef struct {
    const sensor_io_t *io;    /*!< Injected IO (read / now_ms) */
    uint32_t warmup_start_ms; /*!< now_ms() at init */
} sensor_y01_t;

/**
 * @brief Inject IO and record warm-up start time
 */
void sensor_y01_init(sensor_y01_t *ctx, const sensor_io_t *io);

/**
 * @brief Read one sample
 * @param ctx      Driver context
 * @param out      Written only on Y01_OK; left unchanged otherwise
 * @param wait_ms  Hard time budget for the whole call (ms); 0 = do not wait
 * @retval Y01_OK (0) — frame received, checksum OK, warm-up finished
 * @retval Y01_ERR_IO (1) — no frame or bad arguments
 * @retval Y01_ERR_WARMUP (2) — frame OK but still within 5-minute warm-up
 *
 * Warm-up is checked after VERIFY and before writing out so WARMUP
 * means the link is healthy. Shared-UART flush is the caller's job.
 */
uint32_t sensor_y01_get(sensor_y01_t *ctx, sensor_y01_data_t *out, uint32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_Y01_3IN1_H_ */
