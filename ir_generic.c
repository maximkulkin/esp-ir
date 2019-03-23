#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ir_tx.h"
#include "ir_generic.h"


typedef enum {
    ir_state_idle,

    ir_state_header_mark,
    ir_state_header_space,

    ir_state_bit_mark,
    ir_state_bit_space,

    ir_state_footer_mark,
    ir_state_footer_space,
} ir_generic_fsm_state_t;


typedef struct {
    ir_encoder_t encoder;

    ir_generic_config_t *config;

    ir_generic_fsm_state_t fsm_state;

    size_t bit_count;
    size_t bits_left;

    size_t byte_pos;
    uint8_t bit_pos;

    uint8_t data[];
} ir_generic_encoder_t;


static int16_t ir_generic_get_next_pulse(ir_generic_encoder_t *encoder) {
    switch(encoder->fsm_state) {
        case ir_state_idle:
            return 0;

        case ir_state_header_mark:
            encoder->fsm_state = ir_state_header_space;
            return encoder->config->header_mark;

        case ir_state_header_space:
            encoder->fsm_state = ir_state_bit_mark;
            return encoder->config->header_space;

        case ir_state_bit_mark: {
            encoder->fsm_state = ir_state_bit_space;

            uint8_t bit = (encoder->data[encoder->byte_pos] >> encoder->bit_pos) & 0x1;

            return (bit ? encoder->config->bit1_mark : encoder->config->bit0_mark);
        }

        case ir_state_bit_space: {
            encoder->bits_left--;
            if (!encoder->bits_left) {
                if (encoder->config->footer_mark) {
                    encoder->fsm_state = ir_state_footer_mark;
                } else if (encoder->config->footer_space) {
                    encoder->fsm_state = ir_state_footer_space;
                } else {
                    encoder->fsm_state = ir_state_idle;
                }
            } else {
                encoder->fsm_state = ir_state_bit_mark;
            }

            uint8_t bit = (encoder->data[encoder->byte_pos] >> encoder->bit_pos) & 0x1;

            encoder->bit_pos++;
            if (encoder->bit_pos >= 8) {
                encoder->bit_pos = 0;
                encoder->byte_pos++;
            }

            return (bit ? encoder->config->bit1_space : encoder->config->bit0_space);
        }

        case ir_state_footer_mark:
            encoder->fsm_state = ir_state_footer_space;
            return encoder->config->footer_mark;

        case ir_state_footer_space:
            encoder->fsm_state = ir_state_idle;
            return encoder->config->footer_space;
    }

    return 0;
}


static void ir_generic_free(ir_generic_encoder_t *encoder) {
    if (encoder)
        free(encoder);
}


int ir_generic_send(ir_generic_config_t *config, uint8_t *data, uint16_t bit_count) {
    ir_generic_encoder_t *encoder =
        malloc(sizeof(ir_generic_encoder_t) + sizeof(uint8_t) * ((bit_count + 7) >> 3));
    if (!encoder)
        return -1;

    encoder->encoder.get_next_pulse = (ir_get_next_pulse_t)ir_generic_get_next_pulse;
    encoder->encoder.free = (ir_free_t)ir_generic_free;

    encoder->config = config;
    memcpy(encoder->data, data, sizeof(uint8_t) * ((bit_count + 7) >> 3));

    encoder->bit_count = encoder->bits_left = bit_count;
    encoder->byte_pos = encoder->bit_pos = 0;

    if (config->header_mark) {
        encoder->fsm_state = ir_state_header_mark;
    } else if (config->header_space) {
        encoder->fsm_state = ir_state_header_space;
    } else {
        encoder->fsm_state = ir_state_bit_mark;
    }

    int result = ir_tx_send((ir_encoder_t*) encoder);
    if (result) {
        ir_generic_free(encoder);
    }
    return result;
}
