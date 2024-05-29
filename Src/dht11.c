#include "dht11.h"
#include "tim.h"

//extern uint8_t n;
struct DHT11_data arry_data[10];

extern TIM_HandleTypeDef htim3;

void Delay_us1(uint16_t us) {     
	uint16_t differ = 0xffff-us-5;				
	__HAL_TIM_SET_COUNTER(&htim3,differ);	
	HAL_TIM_Base_Start(&htim3);		
	
	while(differ < 0xffff-5){	
		differ = __HAL_TIM_GET_COUNTER(&htim3);	
	}
	HAL_TIM_Base_Stop(&htim3);
}

/*
 *TIM3?????us???
 */
void Delay_us(uint16_t delay)
{
	__HAL_TIM_DISABLE(&htim3);
	__HAL_TIM_SET_COUNTER(&htim3,0);
	__HAL_TIM_ENABLE(&htim3);
	uint16_t curCnt=0;
	while(1)
	{
		curCnt=__HAL_TIM_GET_COUNTER(&htim3);
		if(curCnt>=delay)
			break;
	}
	__HAL_TIM_DISABLE(&htim3);
}


static void mcu_init_output() {
	GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}	

static void mcu_init_input() {
	GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct); 
}

static uint8_t dht11_read_byte() {
	uint8_t data = 0;
	uint8_t retry = 0;
	uint8_t temp;
	for(int i = 0; i < 8; ++i) {
		while(Data_read() == 0 && retry < 100) {
			Delay_us(1);
			++retry;
		}
		if(retry == 100)
			return 0; 
		retry = 0;
		Delay_us(40);
		temp = Data_read() ? 1 : 0;
		
		while(Data_read() == 1 && retry < 100) {
			Delay_us(1);
			++retry;
		}
		if(retry == 100)
			return 0;
		retry = 0;
		data <<= 1;
		data |= temp;
	}
	return data;
}

struct DHT11_data dht11_read() {
	uint8_t retry = 0;
	struct DHT11_data dht11_data; //golbal
	
	mcu_init_output();  //mcu --->0---> dht11
	Data_write_reset();
	HAL_Delay(20);
	Data_write_set();  //mcu --->1---> dht11
	Delay_us(30);
	
	 mcu_init_input(); //dht11 ---> mcu

	Delay_us(20);

	//if(Data_read() == 0) {
			retry = 0;
		while(Data_read() == 0 && retry < 100) {
			Delay_us(1);
			++retry;
		}
		if(retry == 100){
			dht11_data.err = 1;
			return dht11_data;
		}
		retry = 0;
		
		Delay_us(40);
		while(Data_read() == 1 && retry < 100) {
			Delay_us(1);
			++retry;
		}
		if(retry == 100) {
			dht11_data.err = 1;
			return dht11_data;
		}
		retry = 0;
		
		for(int i = 0; i < 5; ++i) {
			dht11_data.data[i] = dht11_read_byte();
		}
		Delay_us(50);	
		
		//check
	uint32_t sum = dht11_data.data[0] + dht11_data.data[1] + dht11_data.data[2] + dht11_data.data[3];
	if(sum == 0) {
		dht11_data.err = 2;
		return dht11_data;
	}
	
	if(dht11_data.data[4] == (uint8_t)sum) {
		dht11_data.temp = dht11_data.data[0];
		dht11_data.temp_f = dht11_data.data[1];
		dht11_data.humidty = dht11_data.data[2];
		dht11_data.humidty_f = dht11_data.data[3];
		dht11_data.err = 0;
		return dht11_data;
	} else {
		dht11_data.err = 1;
		return dht11_data;
	}


}







#define u8 uint8_t
void DHT11_Rst(void)	   
{                 
	mcu_init_output(); 	//SET OUTPUT
    Data_write_reset(); 	//??DQ
    HAL_Delay(20);    	//????18ms
    Data_write_set(); 	//DQ=1 
	Delay_us(30);     	//????20~40us
}


u8 DHT11_Check(void) 	   
{   
	u8 retry=0;
	mcu_init_input();//SET INPUT	 
    while (Data_read() && retry<100)//DHT11???40~80us
	{
		retry++;
		Delay_us(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    while (!Data_read()&&retry<100)//DHT11????????40~80us
	{
		retry++;
		Delay_us(1);
	};
	if(retry>=100)return 1;	    
	return 0;
}

u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(Data_read()&&retry<100)//???????
	{
		retry++;
		Delay_us(1);
	}
	retry=0;
	while(!Data_read()&&retry<100)//??????
	{
		retry++;
		Delay_us(1);
	}
	Delay_us(40);//??40us
	if(Data_read())return 1;
	else return 0;		   
}

u8 DHT11_Read_Byte(void)    
{        
    u8 i,dat;
    dat=0;
	for (i=0;i<8;i++) 
	{
   		dat<<=1; 
	    dat|=DHT11_Read_Bit();
    }						    
    return dat;
}

void DHT11_Read_Data(struct DHT11_data* data)    
{        
 	u8 buf[5];
	u8 i;
	DHT11_Rst();
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++)//??40???
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
		{
			data->humidty = buf[0];  
			data->humidty_f = buf[1];
			data->temp = buf[2];
			data->temp_f = buf[3];
			data->err = 0;
		} else {
			data->err = 1;
		}
		return;
	}
	data->err = 1;
	return;
}












