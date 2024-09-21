/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    hrtim.c
  * @brief   This file provides code for the configuration
  *          of the HRTIM instances.
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
/* Includes ------------------------------------------------------------------*/
#include "hrtim.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

HRTIM_HandleTypeDef hhrtim1;

/* HRTIM1 init function */
void MX_HRTIM1_Init(void)
{

  /* USER CODE BEGIN HRTIM1_Init 0 */

  /* USER CODE END HRTIM1_Init 0 */

  HRTIM_ADCTriggerCfgTypeDef pADCTriggerCfg = {0};
  HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg = {0};
  HRTIM_TimerCfgTypeDef pTimerCfg = {0};
  HRTIM_CompareCfgTypeDef pCompareCfg = {0};
  HRTIM_DeadTimeCfgTypeDef pDeadTimeCfg = {0};
  HRTIM_OutputCfgTypeDef pOutputCfg = {0};

  /* USER CODE BEGIN HRTIM1_Init 1 */

  /* USER CODE END HRTIM1_Init 1 */
  hhrtim1.Instance = HRTIM1;
  hhrtim1.Init.HRTIMInterruptResquests = HRTIM_IT_SYSFLT;
  hhrtim1.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
  if (HAL_HRTIM_Init(&hhrtim1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_DLLCalibrationStart(&hhrtim1, HRTIM_CALIBRATIONRATE_14) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_PollForDLLCalibration(&hhrtim1, 100) != HAL_OK)
  {
    Error_Handler();
  }
  pADCTriggerCfg.UpdateSource = HRTIM_ADCTRIGGERUPDATE_MASTER;
  pADCTriggerCfg.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP2;
  if (HAL_HRTIM_ADCTriggerConfig(&hhrtim1, HRTIM_ADCTRIGGER_1, &pADCTriggerCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pADCTriggerCfg.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERB_CMP2;
  if (HAL_HRTIM_ADCTriggerConfig(&hhrtim1, HRTIM_ADCTRIGGER_3, &pADCTriggerCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pTimeBaseCfg.Period = 16000;
  pTimeBaseCfg.RepetitionCounter = 7;
  pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_MUL32;
  pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
  if (HAL_HRTIM_TimeBaseConfig(&hhrtim1, HRTIM_TIMERINDEX_MASTER, &pTimeBaseCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pTimerCfg.InterruptRequests = HRTIM_MASTER_IT_MREP;
  pTimerCfg.DMARequests = HRTIM_MASTER_DMA_NONE;
  pTimerCfg.DMASrcAddress = 0x0000;
  pTimerCfg.DMADstAddress = 0x0000;
  pTimerCfg.DMASize = 0x1;
  pTimerCfg.HalfModeEnable = HRTIM_HALFMODE_DISABLED;
  pTimerCfg.StartOnSync = HRTIM_SYNCSTART_DISABLED;
  pTimerCfg.ResetOnSync = HRTIM_SYNCRESET_DISABLED;
  pTimerCfg.DACSynchro = HRTIM_DACSYNC_NONE;
  pTimerCfg.PreloadEnable = HRTIM_PRELOAD_ENABLED;
  pTimerCfg.UpdateGating = HRTIM_UPDATEGATING_INDEPENDENT;
  pTimerCfg.BurstMode = HRTIM_TIMERBURSTMODE_MAINTAINCLOCK;
  pTimerCfg.RepetitionUpdate = HRTIM_UPDATEONREPETITION_ENABLED;
  if (HAL_HRTIM_WaveformTimerConfig(&hhrtim1, HRTIM_TIMERINDEX_MASTER, &pTimerCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pCompareCfg.CompareValue = 8000;
  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_MASTER, HRTIM_COMPAREUNIT_3, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pTimeBaseCfg.RepetitionCounter = 0x00;
  if (HAL_HRTIM_TimeBaseConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, &pTimeBaseCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pTimerCfg.InterruptRequests = HRTIM_TIM_IT_NONE;
  pTimerCfg.DMARequests = HRTIM_TIM_DMA_NONE;
  pTimerCfg.PushPull = HRTIM_TIMPUSHPULLMODE_DISABLED;
  pTimerCfg.FaultEnable = HRTIM_TIMFAULTENABLE_NONE;
  pTimerCfg.FaultLock = HRTIM_TIMFAULTLOCK_READWRITE;
  pTimerCfg.DeadTimeInsertion = HRTIM_TIMDEADTIMEINSERTION_ENABLED;
  pTimerCfg.DelayedProtectionMode = HRTIM_TIMER_A_B_C_DELAYEDPROTECTION_DISABLED;
  pTimerCfg.UpdateTrigger = HRTIM_TIMUPDATETRIGGER_TIMER_A;
  pTimerCfg.ResetTrigger = HRTIM_TIMRESETTRIGGER_MASTER_PER;
  pTimerCfg.ResetUpdate = HRTIM_TIMUPDATEONRESET_ENABLED;
  if (HAL_HRTIM_WaveformTimerConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, &pTimerCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pTimerCfg.UpdateTrigger = HRTIM_TIMUPDATETRIGGER_TIMER_B;
  pTimerCfg.ResetTrigger = HRTIM_TIMRESETTRIGGER_MASTER_CMP3;
  if (HAL_HRTIM_WaveformTimerConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, &pTimerCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pCompareCfg.CompareValue = 0xFFDF;
  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pCompareCfg.CompareValue = 8000-128;
  pCompareCfg.AutoDelayedMode = HRTIM_AUTODELAYEDMODE_REGULAR;
  pCompareCfg.AutoDelayedTimeout = 0x0000;

  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_2, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_2, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pCompareCfg.CompareValue = 0xFFDF;
  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_3, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pDeadTimeCfg.Prescaler = HRTIM_TIMDEADTIME_PRESCALERRATIO_MUL8;
  pDeadTimeCfg.RisingValue = 60;
  pDeadTimeCfg.RisingSign = HRTIM_TIMDEADTIME_RISINGSIGN_POSITIVE;
  pDeadTimeCfg.RisingLock = HRTIM_TIMDEADTIME_RISINGLOCK_WRITE;
  pDeadTimeCfg.RisingSignLock = HRTIM_TIMDEADTIME_RISINGSIGNLOCK_WRITE;
  pDeadTimeCfg.FallingValue = 60;
  pDeadTimeCfg.FallingSign = HRTIM_TIMDEADTIME_FALLINGSIGN_POSITIVE;
  pDeadTimeCfg.FallingLock = HRTIM_TIMDEADTIME_FALLINGLOCK_WRITE;
  pDeadTimeCfg.FallingSignLock = HRTIM_TIMDEADTIME_FALLINGSIGNLOCK_WRITE;
  if (HAL_HRTIM_DeadTimeConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, &pDeadTimeCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_DeadTimeConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, &pDeadTimeCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pOutputCfg.Polarity = HRTIM_OUTPUTPOLARITY_HIGH;
  pOutputCfg.SetSource = HRTIM_OUTPUTSET_TIMCMP1;
  pOutputCfg.ResetSource = HRTIM_OUTPUTRESET_TIMCMP3;
  pOutputCfg.IdleMode = HRTIM_OUTPUTIDLEMODE_NONE;
  pOutputCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_INACTIVE;
  pOutputCfg.FaultLevel = HRTIM_OUTPUTFAULTLEVEL_NONE;
  pOutputCfg.ChopperModeEnable = HRTIM_OUTPUTCHOPPERMODE_DISABLED;
  pOutputCfg.BurstModeEntryDelayed = HRTIM_OUTPUTBURSTMODEENTRY_REGULAR;
  if (HAL_HRTIM_WaveformOutputConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1, &pOutputCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_WaveformOutputConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1, &pOutputCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pOutputCfg.SetSource = HRTIM_OUTPUTSET_NONE;
  pOutputCfg.ResetSource = HRTIM_OUTPUTRESET_NONE;
  pOutputCfg.FaultLevel = HRTIM_OUTPUTFAULTLEVEL_INACTIVE;
  if (HAL_HRTIM_WaveformOutputConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA2, &pOutputCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pOutputCfg.FaultLevel = HRTIM_OUTPUTFAULTLEVEL_NONE;
  if (HAL_HRTIM_WaveformOutputConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB2, &pOutputCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_TimeBaseConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, &pTimeBaseCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_3, &pCompareCfg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN HRTIM1_Init 2 */

  /* USER CODE END HRTIM1_Init 2 */
  HAL_HRTIM_MspPostInit(&hhrtim1);

}

void HAL_HRTIM_MspInit(HRTIM_HandleTypeDef* hrtimHandle)
{

  if(hrtimHandle->Instance==HRTIM1)
  {
  /* USER CODE BEGIN HRTIM1_MspInit 0 */

  /* USER CODE END HRTIM1_MspInit 0 */
    /* HRTIM1 clock enable */
    __HAL_RCC_HRTIM1_CLK_ENABLE();

    /* HRTIM1 interrupt Init */
    HAL_NVIC_SetPriority(HRTIM1_Master_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(HRTIM1_Master_IRQn);
  /* USER CODE BEGIN HRTIM1_MspInit 1 */

  /* USER CODE END HRTIM1_MspInit 1 */
  }
}

void HAL_HRTIM_MspPostInit(HRTIM_HandleTypeDef* hrtimHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hrtimHandle->Instance==HRTIM1)
  {
  /* USER CODE BEGIN HRTIM1_MspPostInit 0 */

  /* USER CODE END HRTIM1_MspPostInit 0 */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**HRTIM1 GPIO Configuration
    PA8     ------> HRTIM1_CHA1
    PA9     ------> HRTIM1_CHA2
    PA10     ------> HRTIM1_CHB1
    PA11     ------> HRTIM1_CHB2
    */
    GPIO_InitStruct.Pin = MOS_A_H_Pin|MOS_A_L_Pin|MOS_B_H_Pin|MOS_B_L_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_HRTIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN HRTIM1_MspPostInit 1 */

  /* USER CODE END HRTIM1_MspPostInit 1 */
  }

}

void HAL_HRTIM_MspDeInit(HRTIM_HandleTypeDef* hrtimHandle)
{

  if(hrtimHandle->Instance==HRTIM1)
  {
  /* USER CODE BEGIN HRTIM1_MspDeInit 0 */

  /* USER CODE END HRTIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_HRTIM1_CLK_DISABLE();

    /* HRTIM1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(HRTIM1_Master_IRQn);
  /* USER CODE BEGIN HRTIM1_MspDeInit 1 */

  /* USER CODE END HRTIM1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
