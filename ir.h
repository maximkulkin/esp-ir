#pragma once

typedef struct {
    uint16_t header_mark;
    uint16_t header_space;

    uint16_t bit1_mark;
    uint16_t bit1_space;

    uint16_t bit0_mark;
    uint16_t bit0_space;

    uint16_t footer_mark;
    uint16_t footer_space;
} ir_config_t;

void ir_init(ir_config_t *config);
int ir_send(uint8_t *data, size_t bit_length);
