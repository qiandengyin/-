#ifndef __BSP_74HC165_H__
#define __BSP_74HC165_H__

#include <stdint.h>

void bsp_74hc165d_init(void);
void bsp_74hc165d_read(uint8_t *buffer, int len);

#endif /* __BSP_74HC165_H__ */