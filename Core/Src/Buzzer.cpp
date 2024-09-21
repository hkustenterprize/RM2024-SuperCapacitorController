/**
 * @file buzzer.hpp
 * @author JIANG Yicheng RM2023
 * @brief
 * @version 0.1
 * @date 2023-01-22
 *
 * @copyright Copyright (c) 2023
 */

#include "Buzzer.hpp"

#include "Config.hpp"

namespace Buzzer
{
void init()
{
    BUZZER_TIM.Instance->BUZZER_TIM_CHANNEL_CCR = 0U;
    BUZZER_TIM_PWM_Start(&BUZZER_TIM, BUZZER_TIM_CHANNEL);
}

extern "C"
{
    void TIM1_UP_TIM16_IRQHandler(void)
    {
        __HAL_TIM_CLEAR_FLAG(&BUZZER_TIM, TIM_FLAG_UPDATE);
        __HAL_TIM_DISABLE_IT(&BUZZER_TIM, TIM_IT_UPDATE);
        Buzzer::stop();
    }
}
}  // namespace Buzzer