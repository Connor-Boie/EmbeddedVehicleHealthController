#ifndef APPLICATION_BRIDGE_H
#define APPLICATION_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void application_init(void);
void application_run(void);

void application_timer_interrupt(void);

void application_uart_byte_received(uint8_t byte);
void application_uart_receive_error(void);

#ifdef __cplusplus
}
#endif

#endif
