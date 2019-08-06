#pragma once

#include <stdint.h>

typedef struct ir_decoder ir_decoder_t;

// Decode function: takes decoder and pulses and if decoding is successful should
// populate decoded_data and return decoded data size as result. In case of pulse
// pattern was not recognized, should return 0. Should return negative value if
// error occured.
//
// Args:
//   - decoder - decoder instance (can be used to access decoder configuration)
//   - pulses - an array of detected pulses. Positive values correspond to length of
//       periods of carrier data, negative values - periods of silence.
//   - pulse_count - number of entries in `pulses` array
//   - decoded_data - buffer to receive decoded data. Usually - array of bytes, but
//       can be used to represent high level state encoded in IR transmission (e.g.
//       AC state)
//   - decoded_size - size of `decoded_data` buffer (usually in bytes, but
//       interpretation is up to particular decoder implementation)
//
// Returns:
//   size of decoded data on success; 0 if IR command is not recognized; negative -
//   in case of error
typedef int(*ir_decoder_decode_t)(ir_decoder_t *decoder, int16_t *pulses, uint16_t pulse_count,
                                  void *decoded_data, uint16_t decoded_size);


// Frees decoder
typedef void(*ir_decoder_free_t)(ir_decoder_t *decoder);

// IR decoder
struct ir_decoder {
    ir_decoder_decode_t decode;
    ir_decoder_free_t free;
};


// Initialize IR receiving infra on given GPIO pin, with buffer to capture number of
// pulses given by `buffer_size`. Pulse is period of either carrier or silence.
//
// Args:
//   gpio - GPIO number to receive on
//   buffer_size - size of pulse buffer (maximum number of pulses allowed in one IR
//     command)
void ir_rx_init(uint8_t gpio, uint16_t buffer_size);

// Some sensors have lag and thus marks tend to be slightly longer
// and spaces - slightly shorter. Excess amount (in microseconds) is subtracted from
// mark pulse lengths and added to space lengths.
void ir_rx_set_excess(int16_t excess);

// Start listening for IR transmission until either transmission is matched by
// given decoder (it's decode() function returns positive value) or timeout
// milliseconds has passed.
//
// Args:
//   decoder - decoder to use for decoding IR commands
//   timeout - maximum number of milliseconds to wait for a valid IR command
//   receive_buffer - buffer to store received IR command
//   receive_buffer_size - size of receive buffer (interpretation is up to decoder
//     implementation)
int ir_recv(ir_decoder_t *decoder, uint32_t timeout, void *receive_buffer, uint16_t receive_buffer_size);
