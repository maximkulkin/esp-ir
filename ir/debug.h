#pragma once

#ifdef IR_DEBUG
#define ir_debug(message, ...) printf("IR: " ## message, __VA_ARGS__)
#else
#define ir_debug(message, ...)
#endif
