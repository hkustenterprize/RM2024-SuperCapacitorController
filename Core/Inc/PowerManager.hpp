/**
 * @file PowerManager.hpp
 * @author JIANG Yicheng RM2023
 * @brief
 * @version 0.1
 * @date 2023-01-22
 *
 * @copyright Copyright (c) 2023
 */

#pragma once

#include "Buzzer.hpp"
#include "Calibration.hpp"
#include "Communication.hpp"
#include "Config.hpp"
#include "IncrementalPID.hpp"
#include "MathUtil.hpp"
#include "adc.h"
#include "hrtim.h"
#include "main.h"
#include "tim.h"

#define ERROR_UNDER_VOLTAGE 0b00000001
#define ERROR_OVER_VOLTAGE 0b00000010
#define ERROR_BUCK_BOOST 0b00000100
#define ERROR_SHORT_CIRCUIT 0b00001000
#define ERROR_HIGH_TEMPERATURE 0b00010000
#define ERROR_NO_POWER_INPUT 0b00100000
#define ERROR_CAPACITOR 0b01000000

namespace PowerManager
{

namespace SampleManager
{
struct ADCSample
{
    ADCSample(uint16_t *_buffer, uint16_t _offset);

    uint16_t sum;

    const uint16_t *buffer;
    const uint16_t offset;

    constexpr void sumBuffer();

   private:
    uint16_t temp;
};
struct ProcessedData
{
    float pReferee;
    float iReferee;
    float iChassis;
    float vASide;
    float iASide;
    float vBSide;
    float iBSide;
    float pASide;
    float pBSide;
    float efficiency;

    float temperature;

    float capCharge;
    // I_Referee should have (JudgeSystem -> DCDC_ASide) as positive
    // I_Chassis should have (Chassis -> DCDC_ASide) as positive
    // I_ASide should have (A->B) as positive

    void updateAndCalibrate(const ADCSample &sampleVASide,
                            const ADCSample &sampleVBSide,
                            const ADCSample &sampleIReferee,
                            const ADCSample &sampleIASide,
                            const ADCSample &sampleIBSide);

    static ProcessedData processedData;

   private:
    ProcessedData();
    float lastIJudge;
    float lastIBSide;
    float lastIASide;
};
}  // namespace SampleManager

struct ControlData
{
    float refereePowerLimit;
    float energyRemain;
    bool enableOutput;

    static ControlData controlData;

   private:
    ControlData();
};

struct Status
{
    bool outputEnabled;
    uint8_t errorCode;
    float capEnergy;
    float chassisPower;
    float chassisPowerLimit;
    bool lowBattery;
    float realVBToVA;

    uint16_t communicationTimeoutCnt = 0;

    static Status status;

   private:
    Status();
};

void init();

void updateEnergy();

void systemRestart();

}  // namespace PowerManager