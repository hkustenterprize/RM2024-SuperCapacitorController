/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define I_SENSE_CAP_Pin GPIO_PIN_1
#define I_SENSE_CAP_GPIO_Port GPIOC
#define SENSE_THERMAL_Pin GPIO_PIN_2
#define SENSE_THERMAL_GPIO_Port GPIOC
#define V_SENSE_B_Pin GPIO_PIN_3
#define V_SENSE_B_GPIO_Port GPIOC
#define V_SENSE_A_Pin GPIO_PIN_0
#define V_SENSE_A_GPIO_Port GPIOA
#define I_SENSE_JUDGE_Pin GPIO_PIN_1
#define I_SENSE_JUDGE_GPIO_Port GPIOA
#define I_SENSE_A_Pin GPIO_PIN_2
#define I_SENSE_A_GPIO_Port GPIOA
#define TIM1_CH2N_BUZZER_Pin GPIO_PIN_0
#define TIM1_CH2N_BUZZER_GPIO_Port GPIOB
#define MOS_A_H_Pin GPIO_PIN_8
#define MOS_A_H_GPIO_Port GPIOA
#define MOS_A_L_Pin GPIO_PIN_9
#define MOS_A_L_GPIO_Port GPIOA
#define MOS_B_H_Pin GPIO_PIN_10
#define MOS_B_H_GPIO_Port GPIOA
#define MOS_B_L_Pin GPIO_PIN_11
#define MOS_B_L_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
