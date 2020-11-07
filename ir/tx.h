#pragma once

#include <stdint.h>


typedef struct ir_encoder ir_encoder_t;

// Function to get next pulse to transmit. Pulse is period (in microseconds) of
// either carrier transmission (positive value) or silence (negative value).
typedef int16_t(*ir_get_next_pulse_t)(ir_encoder_t *);

// Free given encoder
typedef void(*ir_free_t)(ir_encoder_t *);

// IR encoder - sending state + functions to access it.
// Usually, an instance of encoder is created for every IR transmission and freed
// when all data is transmitted.
//
// Encoders is a way to represent transmitted IR command in a more compact way,
// rather than storing every pulse in an array (effective memory usage).
struct ir_encoder {
    ir_get_next_pulse_t get_next_pulse;
    ir_free_t free;
    int16_t carry;  // carry pulse to workaround 10K timer bug
};


// Initialize IR transmission system. Always uses GPIO14 for transmission.
void ir_tx_init();

// Send IR command represented by given encoder
//
// Args:
//   - encoder - encoder representing IR command
//
// Returns:
//   0 on success; negative value - error code
int ir_tx_send(ir_encoder_t *encoder);
