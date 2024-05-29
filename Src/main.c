/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "usart.h"
#include "gpio.h"
#include "socket.h"
#include <stdio.h>
#include "dht11.h"
//#include <tcp.h>
#include "tim.h"
#include "relay.h"
#include "bh1750.h"
#include "adc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void tcp_task();
void dht11_task();
void bh1750_task();
void led() {
	//while(1) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	//}
}


/* USER CODE END 0 */


struct all_data{
	struct DHT11_data dht11;
	
	uint16_t bh1750;
};

struct config_data{
	uint8_t temp;
	uint8_t humd;
	
	uint8_t Soil_Moisture;
	int isConfig;
};

struct all_data scada;
struct config_data config;

int get_config_data(char* buffer, struct config_data* data) {  //"t:34&h:55$" == *10t:34&h:55$
	if(buffer[0] == 't' && buffer[1] == ':' && buffer[4] == '&' && buffer[5] == 'h' && buffer[6] == ':') {
		data->temp = (buffer[2] - '0') * 10 + (buffer[3] - '0');
		data->humd = (buffer[7] - '0') * 10 + (buffer[8] - '0');
		config.isConfig = 1;
		return 0;
	}
	return -1;
}

int write_dht11_buffer(char* buffer, size_t size) {
	sprintf(buffer, "*20t:%d.%d&h:%d.%d$", scada.dht11.temp,  scada.dht11.temp_f, scada.dht11.humidty, scada.dht11.humidty_f);
	return 0;
}

int write_bh1750_buffer(char* buffer, size_t size) {
	sprintf(buffer, "*21b:%d$", scada.bh1750);
	return 0;
}

void start_water(int time) { //default 5s
	relay_reset();
	HAL_Delay(time);
	relay_set();
	return;
}

int check_data(char* buffer, char* buf, size_t size) {  
	if(buffer[0] != '*' || buffer[1] == '0') {
		sprintf(buf, "data format err\n");
		return -1;
	}

	if(buffer[1] == '1') {  
    //config
		if(get_config_data(buffer + 3, &config) < 0) {
			sprintf(buf, "*!config err\n");
			return -1;
		}
		sprintf(buf, "**$");
	} else if(buffer[1] == '2') {  
		//scada
		if(buffer[2] == '0') {
			
			write_dht11_buffer(buf, 10);
		} else if(buffer[2] == '1') {
			
			write_bh1750_buffer(buf, 10);
		} else if(buffer[2] == '2') {
			uint32_t data = getHumdityValue();
			sprintf(buf, "*22M:%d$", (4096 - data) / 4096);
		} else {
			sprintf(buf, "scada err\n");
			return -1;
		}
	}
	return 0;
}



uint8_t mm[2] = {0, 0};

void bh1750_test(unsigned char* buffer, uint8_t size) {
	bh1750_write(0x01);  //
	bh1750_write(0x07);
	bh1750_write(0x20);
	HAL_Delay(180);
	bh1750_read(buffer, size);
}

void testt() {
	HAL_Delay(1000);
}

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();

  MX_USART1_UART_Init();
	
  MX_FREERTOS_Init(); 
	
	MX_TIM3_Init();
	
	MX_I2C1_Init();
	
	MX_ADC1_Init();
	
	relay_init();
	

	
	//bh1750_test(mm, sizeof(mm));  //test
	
	config.isConfig = 0;

	


	xTaskCreate(dht11_task, "dht11", 500, NULL, 1, NULL);
	//xTaskCreate(bh1750_task, "bh1750", 200, NULL, 1, NULL);
	//xTaskCreate(led, "led", 500, NULL, 1, NULL);
	xTaskCreate(tcp_task, "tcp", 500, NULL, 1, NULL);
	//xTaskCreate(testt, "test", 500, NULL, 1, NULL);
  osKernelStart();
 
  while (1)
  {

  }
}
//-----------------------------------------//
void bh1750_task() {
	while(1) {
		bh1750_write(0x01);  //
		bh1750_write(0x07);
		bh1750_write(0x20);
		HAL_Delay(180);
		
		uint8_t buffer[2] = {0};
		bh1750_read(buffer, sizeof(buffer));
		
		scada.bh1750 = ((uint16_t)buffer[0] << 8) | buffer[1];
		HAL_Delay(2000);  //2s
	}
}

void dht11_task() {
	while(1) {
		//dht11_task
		for(int i = 0; i < 4; ++i) {
			DHT11_Read_Data(&(scada.dht11));
			if(scada.dht11.err == 0 && config.isConfig == 1 && config.humd > scada.dht11.humidty) {
				led();
			//sprintf(test, "cof:%d, scd:%d, now:%d", config.humd, scada.dht11.humidty, data.humidty);
				start_water(5000);
				continue;
			}
			HAL_Delay(500);
		}
		//bh1750_task
		bh1750_write(0x01);  //
		bh1750_write(0x07);
		bh1750_write(0x20);
		HAL_Delay(180);
		
		uint8_t buffer[2] = {0};
		bh1750_read(buffer, sizeof(buffer));
		
		scada.bh1750 = ((uint16_t)buffer[0] << 8) | buffer[1];
		HAL_Delay(100);
	}
}


void tcp_task() {
	struct sockaddr_in addr, addr1;
	int sock1 = socket(AF_INET, SOCK_STREAM, 0);
	if(sock1 < 0)
		return;
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sock < 0 ) 
		return ;
	if(sock  == 0 )
		return;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(100);
	addr.sin_addr.s_addr = inet_addr("192.168.111.10");
	
	if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) 
		return ;
	
	if(listen(sock, 100) < 0) {

	}
	//led();
	int size = sizeof(addr1);

	
	while(1) {

		int newconn = accept(sock, (struct sockaddr*)&addr1, (size_t*)&size);

	  if(newconn < 0) {
			continue;
	  }
		if(newconn > 0) {
			int len = 1024;
			char recv_buf[24]; //recv
			char send_buf[24];

			
			while(1) {
				memset(send_buf, 0, sizeof(send_buf));
				memset(recv_buf, 0, sizeof(recv_buf));
				int ret = read(newconn, recv_buf, sizeof(recv_buf));
				if(ret < 1) {
					break;
				}
				//check data;
				
				check_data(recv_buf, send_buf, sizeof(send_buf));
				//uint32_t data = getHumdityValue();
				//sprintf(send_buf, "hum:%d", data);
			
				write(newconn, send_buf, sizeof(send_buf));
				
				//write(newconn, test, sizeof(test));
				}
		}
		close(newconn);
	}
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
