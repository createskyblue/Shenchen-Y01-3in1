/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor_y01_3in1.c
 * @brief   Shenchen Y01-3in1 multi-gas module driver implementation
 *
 * Vendor: Shenzhen Shenchen Technology Co., LTD
 * Model:  Y01-3in1 (TVOC / CH2O / CO2)
 *
 * SEEK → RECV → VERIFY state machine. Results go only to the caller's out
 * buffer. This driver does not flush UART.
 ******************************************************************************
 */

#include "sensor_y01_3in1.h"

/* Frame field indices */
#define Y01_IDX_ADDR_HI   0u
#define Y01_IDX_ADDR_LO   1u
#define Y01_IDX_TVOC_HI   2u
#define Y01_IDX_TVOC_LO   3u
#define Y01_IDX_CH2O_HI   4u
#define Y01_IDX_CH2O_LO   5u
#define Y01_IDX_CO2_HI    6u
#define Y01_IDX_CO2_LO    7u
#define Y01_IDX_CHECK     8u

typedef enum {
    Y01_ST_SEEK = 0,
    Y01_ST_RECV,
    Y01_ST_VERIFY
} y01_state_t;

void sensor_y01_init(sensor_y01_t *ctx, const sensor_io_t *io)
{
    ctx->io = io;
    ctx->warmup_start_ms = (io != NULL && io->now_ms != NULL) ? io->now_ms() : 0u;
}

uint32_t sensor_y01_get(sensor_y01_t *ctx, sensor_y01_data_t *out, uint32_t wait_ms)
{
    uint8_t     frame[Y01_FRAME_LEN];
    uint8_t     idx = 0u;
    uint8_t     sum;
    uint8_t     i;
    y01_state_t state = Y01_ST_SEEK;
    uint32_t    deadline;

    if (ctx == NULL || out == NULL || ctx->io == NULL ||
        ctx->io->read == NULL || ctx->io->now_ms == NULL) {
        return Y01_ERR_IO;
    }

    deadline = ctx->io->now_ms() + wait_ms;

    do {
        uint8_t byte;

        if (ctx->io->read(&byte, 1u) != 1u) {
            continue;
        }

        switch (state) {
        case Y01_ST_SEEK:
            if (byte != Y01_ADDR_HI) {
                break;
            }
            frame[0] = byte;
            idx      = 1u;
            state    = Y01_ST_RECV;
            break;

        case Y01_ST_RECV:
            frame[idx++] = byte;
            if (idx < Y01_FRAME_LEN) {
                break;
            }
            state = Y01_ST_VERIFY;
            /* FALLTHRU */

        case Y01_ST_VERIFY:
            /* Address 2C E4; checksum = plain sum of B0..B7 */
            if (frame[Y01_IDX_ADDR_LO] != Y01_ADDR_LO) {
                state = Y01_ST_SEEK;
                idx   = 0u;
                break;
            }
            sum = 0u;
            for (i = Y01_IDX_ADDR_HI; i <= Y01_IDX_CO2_LO; i++) {
                sum = (uint8_t)(sum + frame[i]);
            }
            if (sum != frame[Y01_IDX_CHECK]) {
                state = Y01_ST_SEEK;
                idx   = 0u;
                break;
            }
            /* Frame accepted — warm-up here so it is not confused with I/O errors */
            if ((uint32_t)(ctx->io->now_ms() - ctx->warmup_start_ms) < Y01_WARMUP_MS) {
                return Y01_ERR_WARMUP;
            }
            out->tvoc_mg_m3 = (float)(((uint16_t)frame[Y01_IDX_TVOC_HI] << 8) |
                                       (uint16_t)frame[Y01_IDX_TVOC_LO]) * 0.001f;
            out->ch2o_mg_m3 = (float)(((uint16_t)frame[Y01_IDX_CH2O_HI] << 8) |
                                       (uint16_t)frame[Y01_IDX_CH2O_LO]) * 0.001f;
            out->co2_ppm    = (float)(((uint16_t)frame[Y01_IDX_CO2_HI] << 8) |
                                       (uint16_t)frame[Y01_IDX_CO2_LO]);
            out->valid      = 1u;
            return Y01_OK;
        }
    } while ((int32_t)(ctx->io->now_ms() - deadline) < 0);

    return Y01_ERR_IO;
}
