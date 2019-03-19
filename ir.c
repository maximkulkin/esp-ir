#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// The following definitions is taken from ESP8266_MP3_DECODER demo
// https://github.com/espressif/ESP8266_MP3_DECODER/blob/master/mp3/driver/i2s_freertos.c
// It is requred to set clock to I2S subsystem
void sdk_rom_i2c_writeReg_Mask(uint32_t block, uint32_t host_id,
        uint32_t reg_add, uint32_t Msb, uint32_t Lsb, uint32_t indata);

#ifndef i2c_bbpll
#define i2c_bbpll                               0x67
#define i2c_bbpll_en_audio_clock_out            4
#define i2c_bbpll_en_audio_clock_out_msb        7
#define i2c_bbpll_en_audio_clock_out_lsb        7
#define i2c_bbpll_hostid                        4
#endif


#include <esp/timer.h>
#include <esp/i2s_regs.h>
#include <common_macros.h>
#include <esp/gpio.h>

#define IR_GPIO_NUM 14  // MTCK pin (I2S CLK pin)

#include "ir.h"

typedef enum {
    ir_state_idle,

    ir_state_header_mark,
    ir_state_header_space,

    ir_state_bit_mark,
    ir_state_bit_space,

    ir_state_footer_mark,
    ir_state_footer_space,
} ir_fsm_state_t;


typedef struct {
    ir_fsm_state_t fsm_state;

    uint8_t *data;
    size_t bit_length;
    size_t bits_left;

    size_t byte_pos;
    uint8_t bit_pos;
} ir_state_t;


static volatile ir_config_t ir_config;
static volatile ir_state_t ir_state;


static void gen_carrier() {
    iomux_set_function(gpio_to_iomux(IR_GPIO_NUM), IOMUX_GPIO14_FUNC_I2SI_WS);

    I2S.CONF = SET_MASK_BITS(I2S.CONF, I2S_CONF_RX_START);
}


static void clr_carrier() {
    gpio_enable(IR_GPIO_NUM, GPIO_OUTPUT);
    gpio_write(IR_GPIO_NUM, 0);
    I2S.CONF = CLEAR_MASK_BITS(I2S.CONF, I2S_CONF_RX_START);
}


static inline void hw_timer_init(void (*isr)(void*), void *arg) {
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);

    _xt_isr_attach(INUM_TIMER_FRC1, isr, arg);

    timer_set_interrupts(FRC1, true);
}


static inline void hw_timer_pause() {
    timer_set_run(FRC1, false);
}


static inline void hw_timer_arm(uint32_t us) {
    timer_set_timeout(FRC1, us);
    timer_set_run(FRC1, true);
}


static void IRAM ir_timer_handler(void *arg) {
    hw_timer_pause();

    switch(ir_state.fsm_state) {
        case ir_state_idle:
            // TODO: signal that transmission is over
            break;

        case ir_state_header_mark:
            ir_state.fsm_state = ir_state_header_space;

            gen_carrier();
            hw_timer_arm(ir_config.header_mark);
            break;

        case ir_state_header_space:
            ir_state.fsm_state = ir_state_bit_mark;

            clr_carrier();
            hw_timer_arm(ir_config.header_space);
            break;

        case ir_state_bit_mark: {
            ir_state.fsm_state = ir_state_bit_space;

            uint8_t bit = (ir_state.data[ir_state.byte_pos] >> ir_state.bit_pos) & 0x1;

            gen_carrier();
            hw_timer_arm(bit ? ir_config.bit1_mark : ir_config.bit0_mark);
            break;
        }

        case ir_state_bit_space: {
            ir_state.bits_left--;
            if (!ir_state.bits_left) {
                if (ir_config.footer_mark) {
                    ir_state.fsm_state = ir_state_footer_mark;
                } else if (ir_config.footer_space) {
                    ir_state.fsm_state = ir_state_footer_space;
                } else {
                    ir_state.fsm_state = ir_state_idle;
                }
            } else {
                ir_state.fsm_state = ir_state_bit_mark;
            }

            uint8_t bit = (ir_state.data[ir_state.byte_pos] >> ir_state.bit_pos) & 0x1;

            ir_state.bit_pos++;
            if (ir_state.bit_pos >= 8) {
                ir_state.bit_pos = 0;
                ir_state.byte_pos++;
            }

            clr_carrier();
            hw_timer_arm(bit ? ir_config.bit1_space : ir_config.bit0_space);
            break;
        }

        case ir_state_footer_mark:
            ir_state.fsm_state = ir_state_footer_space;

            gen_carrier();
            hw_timer_arm(ir_config.footer_mark);
            break;

        case ir_state_footer_space:
            ir_state.fsm_state = ir_state_idle;

            clr_carrier();
            hw_timer_arm(ir_config.footer_space);
            break;
    }
}


void ir_init(ir_config_t *config) {
    // Start I2C clock for I2S subsystem
    sdk_rom_i2c_writeReg_Mask(
        i2c_bbpll, i2c_bbpll_hostid,
        i2c_bbpll_en_audio_clock_out,
        i2c_bbpll_en_audio_clock_out_msb,
        i2c_bbpll_en_audio_clock_out_lsb,
        1
    );

    // Clear I2S subsystem
    CLEAR_MASK_BITS(I2S.CONF, I2S_CONF_RESET_MASK);
    SET_MASK_BITS(I2S.CONF, I2S_CONF_RESET_MASK);
    CLEAR_MASK_BITS(I2S.CONF, I2S_CONF_RESET_MASK);

    // Set i2s clk freq 
    I2S.CONF = SET_FIELD(I2S.CONF, I2S_CONF_BCK_DIV, 62);
    I2S.CONF = SET_FIELD(I2S.CONF, I2S_CONF_CLKM_DIV, 2);
    I2S.CONF = SET_FIELD(I2S.CONF, I2S_CONF_BITS_MOD, 0);

    hw_timer_init(ir_timer_handler, NULL);

    ir_config = *config;
}


int ir_send(uint8_t *data, size_t bit_length) {
    if (bit_length == 0)
        return -2;

    size_t byte_length = (bit_length + 7) >> 3;

    // TODO: free .data when transmission is over
    if (ir_state.data)
        free(ir_state.data);

    ir_state.data = malloc(byte_length);
    if (!ir_state.data)
        return -1;

    memcpy(ir_state.data, data, byte_length);

    ir_state.bit_length = bit_length;
    ir_state.bits_left = bit_length;
    ir_state.byte_pos = 0;
    ir_state.bit_pos = 0;
    if (ir_config.header_mark) {
        ir_state.fsm_state = ir_state_header_mark;
    } else if (ir_config.header_space) {
        ir_state.fsm_state = ir_state_header_space;
    } else {
        ir_state.fsm_state = ir_state_bit_mark;
    }
    ir_timer_handler(NULL);

    return 0;
}

