/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include <stdio.h>
static const uint8_t MPUADDR = 0x68 << 1;
static const uint8_t REG_TEMP = 0x41;
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_LPUART1_UART_Init(void);
void i2c_scan(void);
static HAL_StatusTypeDef wake_up_mpu(void);
static HAL_StatusTypeDef get_temp(float *temp_celcius);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */

static HAL_StatusTypeDef wake_up_mpu(void)
{
	uint8_t w[] = {0x6B, 0x00};
	return HAL_I2C_Master_Transmit(&hi2c1, MPUADDR, w, 2, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef get_temp(float *temp_celcius)
{
	//get instructions for reading temperature
	uint8_t raw[2]= {0x41, 0x42};

	HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c1, MPUADDR, 0x41, I2C_MEMADD_SIZE_8BIT, raw, 2, HAL_MAX_DELAY);

	if (st != HAL_OK)
	{
		return st;
	}

	uint16_t counts = (uint16_t)( (raw[0] << 8) | raw[1]);

    *temp_celcius = (counts / 333.87f) + 21.0f;      // MPU-9250 temp conversion
    return HAL_OK;
}

int main(void) {

	HAL_Init();

	SystemClock_Config();

	MX_GPIO_Init();
	MX_LPUART1_UART_Init();
	MX_I2C1_Init();

	i2c_scan();

	while (1)
	{
		float tc;
		if (wake_up_mpu() == HAL_OK && get_temp(&tc) == HAL_OK)
		{
			char line[32];
			int32_t centi = (int32_t)lroundf(tc * 100.0f);   // temperature ×100
			int neg = centi < 0; if (neg) centi = -centi;
			uint32_t whole = (uint32_t)(centi / 100);
			uint32_t frac  = (uint32_t)(centi % 100);

			// build "T=±W.FF C\r\n" without %f
			int n = 0;
			line[n++] = 'T'; line[n++] = '='; if (neg) line[n++] = '-';

			// write whole part
			char tmp[10]; int i=0; do { tmp[i++] = '0' + (whole % 10); whole/=10; } while (whole);
			while (i--) line[n++] = tmp[i];

			line[n++] = '.';
			line[n++] = '0' + (frac/10);
			line[n++] = '0' + (frac%10);
			line[n++] = ' '; line[n++] = 'C';
			line[n++] = '\r'; line[n++] = '\n';

			HAL_UART_Transmit(&hlpuart1, (uint8_t*)line, n, HAL_MAX_DELAY);
		}
		else
		{
			const char *err = "I2C error\r\n";
			HAL_UART_Transmit(&hlpuart1, (uint8_t*)err, strlen(err), HAL_MAX_DELAY);
		}
	}
}


void i2c_scan(void) {
    char line[32];
    for (uint8_t a = 1; a < 127; a++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, a<<1, 2, 5) == HAL_OK) {
            int n = snprintf(line, sizeof(line), "Found: 0x%02X\r\n", a);
            HAL_UART_Transmit(&hlpuart1, (uint8_t*)line, n, HAL_MAX_DELAY);
        }
    }
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)"Scan done\r\n", 11, HAL_MAX_DELAY);
}
		/**
		 * @brief System Clock Configuration
		 * @retval None
		 */
		void SystemClock_Config(void) {
			RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
			RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

			/** Configure the main internal regulator output voltage
			 */
			if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1)
					!= HAL_OK) {
				Error_Handler();
			}

			/** Initializes the RCC Oscillators according to the specified parameters
			 * in the RCC_OscInitTypeDef structure.
			 */
			RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
			RCC_OscInitStruct.MSIState = RCC_MSI_ON;
			RCC_OscInitStruct.MSICalibrationValue = 0;
			RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
			RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
			if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
				Error_Handler();
			}

			/** Initializes the CPU, AHB and APB buses clocks
			 */
			RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK
					| RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
					| RCC_CLOCKTYPE_PCLK2;
			RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
			RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
			RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
			RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

			if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0)
					!= HAL_OK) {
				Error_Handler();
			}
		}

		/**
		 * @brief I2C1 Initialization Function
		 * @param None
		 * @retval None
		 */
		static void MX_I2C1_Init(void) {

			/* USER CODE BEGIN I2C1_Init 0 */

			/* USER CODE END I2C1_Init 0 */

			/* USER CODE BEGIN I2C1_Init 1 */

			/* USER CODE END I2C1_Init 1 */
			hi2c1.Instance = I2C1;
			hi2c1.Init.Timing = 0x00100D14;
			hi2c1.Init.OwnAddress1 = 0;
			hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
			hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
			hi2c1.Init.OwnAddress2 = 0;
			hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
			hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
			hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
			if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
				Error_Handler();
			}

			/** Configure Analogue filter
			 */
			if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)
					!= HAL_OK) {
				Error_Handler();
			}

			/** Configure Digital filter
			 */
			if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
				Error_Handler();
			}
			/* USER CODE BEGIN I2C1_Init 2 */

			/* USER CODE END I2C1_Init 2 */

		}

		/**
		 * @brief LPUART1 Initialization Function
		 * @param None
		 * @retval None
		 */
		static void MX_LPUART1_UART_Init(void) {

			/* USER CODE BEGIN LPUART1_Init 0 */

			/* USER CODE END LPUART1_Init 0 */

			/* USER CODE BEGIN LPUART1_Init 1 */

			/* USER CODE END LPUART1_Init 1 */
			hlpuart1.Instance = LPUART1;
			hlpuart1.Init.BaudRate = 115200;
			hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
			hlpuart1.Init.StopBits = UART_STOPBITS_1;
			hlpuart1.Init.Parity = UART_PARITY_NONE;
			hlpuart1.Init.Mode = UART_MODE_TX_RX;
			hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
			hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
			hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
			if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
				Error_Handler();
			}
			/* USER CODE BEGIN LPUART1_Init 2 */

			/* USER CODE END LPUART1_Init 2 */

		}

		/**
		 * @brief GPIO Initialization Function
		 * @param None
		 * @retval None
		 */
		static void MX_GPIO_Init(void) {
			GPIO_InitTypeDef GPIO_InitStruct = { 0 };
			/* USER CODE BEGIN MX_GPIO_Init_1 */

			/* USER CODE END MX_GPIO_Init_1 */

			/* GPIO Ports Clock Enable */
			__HAL_RCC_GPIOA_CLK_ENABLE();
			__HAL_RCC_GPIOG_CLK_ENABLE();
			HAL_PWREx_EnableVddIO2();
			__HAL_RCC_GPIOB_CLK_ENABLE();

			/*Configure GPIO pin Output Level */
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

			/*Configure GPIO pin Output Level */
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

			/*Configure GPIO pin : PA3 */
			GPIO_InitStruct.Pin = GPIO_PIN_3;
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

			/*Configure GPIO pin : PB5 */
			GPIO_InitStruct.Pin = GPIO_PIN_5;
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

			/* USER CODE BEGIN MX_GPIO_Init_2 */

			/* USER CODE END MX_GPIO_Init_2 */
		}

		/* USER CODE BEGIN 4 */

		/* USER CODE END 4 */

		/**
		 * @brief  This function is executed in case of error occurrence.
		 * @retval None
		 */
		void Error_Handler(void) {
			/* USER CODE BEGIN Error_Handler_Debug */
			/* User can add his own implementation to report the HAL error return state */
			__disable_irq();
			while (1) {
			}
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
