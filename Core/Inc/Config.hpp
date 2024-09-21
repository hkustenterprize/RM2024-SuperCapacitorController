/**
 * @file Config.hpp
 * @author JIANG Yicheng RM2023
 * @brief
 * @version 0.1
 * @date 2023-01-23
 *
 * @copyright Copyright (c) 2023
 */

#define MAX_CAP_VOLTAGE 28.8f

#define SHORT_CIRCUIT_CURRENT 15.5f
#define SHORT_CIRCUIT_VOLTAGE 4.0f

#define I_LIMIT 22.5f

#define OVER_TEMP_TRIGGER_THRESHOLD 180
#define OVER_TEMP_RECOVER_THRESHOLD 200

#define LOW_BATTERY_VOLTAGE 20.92f
#define LOW_BATTERY_RECOVER_VOLTAGE 21.6f

#define DEFAULT_REFEREE_POWER 37.0f

#define ENERGY_BUFFER 50.0f

#define ADC_CHANNEL_COUNT 3
#define ADC_SAMPLE_COUNT 8
#define DIVIDED_BY_ADC_SAMPLE_COUNT (1.0f / ADC_SAMPLE_COUNT)

#define ADC_FILTER_ALPHA 0.5f

#define HRTIM_PERIOD 16000

#pragma once

#define configASSERT(x) \
    if (!(x))           \
        __asm volatile("bkpt ");
