/**
 * @file buzzer.hpp
 * @author JIANG Yicheng RM2023
 * @brief
 * @version 0.1
 * @date 2023-01-22
 *
 * @copyright Copyright (c) 2023
 */

#pragma once

#include "main.h"
#include "tim.h"

#define BUZZER_TIM htim1
#define BUZZER_TIM_CHANNEL TIM_CHANNEL_2
#define BUZZER_TIM_CHANNEL_CCR CCR2
#define BUZZER_TIM_PWM_Start HAL_TIMEx_PWMN_Start

namespace Buzzer
{
/**
 * @brief Initialize the buzzer
 */
void init();

/**
 * @brief Play a tone
 *
 * @param frequency Frequency of the tone
 */
inline void play(uint32_t frequency)
{
    BUZZER_TIM.Instance->ARR                    = (uint32_t)(50000U / frequency) - 1U;
    BUZZER_TIM.Instance->BUZZER_TIM_CHANNEL_CCR = BUZZER_TIM.Instance->ARR / 2U;
    BUZZER_TIM.Instance->CNT                    = 0U;
}

/**
 * @brief Play a tone
 *
 * @param frequency Frequency of the tone
 * @param duration Duration of the tone
 */
inline void play(uint32_t frequency, uint32_t duration)
{
    BUZZER_TIM.Instance->RCR                    = (duration * frequency / 1000U) - 1U;
    BUZZER_TIM.Instance->EGR                    = TIM_EGR_UG;
    BUZZER_TIM.Instance->ARR                    = (50000U / frequency) - 1U;
    BUZZER_TIM.Instance->BUZZER_TIM_CHANNEL_CCR = BUZZER_TIM.Instance->ARR / 2U;
    BUZZER_TIM.Instance->CNT                    = 0U;
    __HAL_TIM_CLEAR_FLAG(&BUZZER_TIM, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&BUZZER_TIM, TIM_IT_UPDATE);
}

/**
 * @brief Stop playing the tone
 */
inline void stop() { BUZZER_TIM.Instance->BUZZER_TIM_CHANNEL_CCR = 0U; }
}  // namespace Buzzer