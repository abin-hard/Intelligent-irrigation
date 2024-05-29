#include "bh1750.h"

void bh1750_write( uint8_t data ) {
	HAL_I2C_Master_Transmit(&hi2c1, 0x46, &data, 1, 1000);  //write 1 byte data to IIC dev
}
void bh1750_read(uint8_t* buffer, uint8_t size) {
	HAL_I2C_Master_Receive(&hi2c1, 0x46, buffer, size, 1000);
}