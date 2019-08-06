#pragma once

#include <stdint.h>
#include <ir/rx.h>


// NEC protocol decoder
//
// NEC protocol command consists of
// - header (long carrier pulse (mark) + long silence (space))
// - sequence of pulses encoding bits
//   - zeros are encoded as short carrier pulse (mark) + short silence (space)
//   - ones are encoded as short carrier pulse (mark) + long silence (space)
// - footer (short carrier pulse + long silence)
//
// Different devices have different periods for NEC protocol components, this
// structure allows to configure all those periods.
typedef struct {
    int16_t header_mark;
    int16_t header_space;

    int16_t bit1_mark;
    int16_t bit1_space;

    int16_t bit0_mark;
    int16_t bit0_space;

    int16_t footer_mark;
    int16_t footer_space;

    // Number of percents by which real value can differ +/- to be still considered a
    // match. E.g. for a mark of 565us 10% tolerance would allow
    // 565+/-56us -> 509 to 630us range
    uint8_t tolerance;
} ir_generic_config_t;


// Send NEC-like IR command
//
// Args:
//   - config - NEC protocol config
//   - data - data bytes to send
//   - data_size - number of bytes to send
//
// Returns:
//   0 on success; negative value - error code
int ir_generic_send(ir_generic_config_t *config, uint8_t *data, uint16_t data_size);
ir_decoder_t *ir_generic_make_decoder(ir_generic_config_t *config);
