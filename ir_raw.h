#pragma once


int ir_raw_send(int16_t *widths, uint16_t count);
int ir_raw_recv(uint32_t timeout, void *recieved_data, uint16_t *received_size);
