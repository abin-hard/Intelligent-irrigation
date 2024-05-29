#ifndef __BH1750_H__
#define __BH1750_H__

#include "main.h"
#include "i2c.h"

void bh1750_write(uint8_t data );
void bh1750_read( uint8_t* buffer, uint8_t size);

#endif 
