#ifndef __DHT11_H__
#define __DHT11_H__

#include "main.h"

#define Data_write_set() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET) //output higt
#define Data_write_reset() HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET) //output low

#define Data_read() HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) //input

struct DHT11_data {
	uint8_t data[5];
	uint8_t temp;
	uint8_t temp_f;
	uint8_t humidty;
	uint8_t humidty_f;
	uint8_t err;
};

//struct DHT11_data dht11_read();
void Delay_us(uint16_t us);

void DHT11_Read_Data(struct DHT11_data* data);  

#endif