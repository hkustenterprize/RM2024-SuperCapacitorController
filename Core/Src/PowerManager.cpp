#include "PowerManager.hpp"

namespace PowerManager
{

ControlData ControlData::controlData;

ControlData::ControlData() : refereePowerLimit(43.0f), energyRemain(10.0f), enableOutput(false) {}

Status Status::status;

Status::Status() : outputEnabled(false), errorCode(0), capEnergy(0.0f), chassisPower(0.0f), chassisPowerLimit(0.0f) {}

struct TempData
{
    float outputDuty            = 1.0f;
    float baseRefereePower      = DEFAULT_REFEREE_POWER;
    float targetRefereePower    = DEFAULT_REFEREE_POWER;
    float lastRefereePowerLimit = DEFAULT_REFEREE_POWER;
    float targetAPower          = 0.0f;
    float targetIA              = 0.0f;
    uint16_t ledBlinkCnt        = 0;
} tempData;

namespace TimerManager
{

static inline void HRTIMStartPWM()
{
    /* start HRTIM PWM */
    HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_MASTER);
    HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A);
    HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_B);
}

static inline void HRTIMConfig()
{
    __HAL_HRTIM_MASTER_ENABLE_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);  // enable master repetition interrupt
}

static inline void TIMConfig()
{
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);  // enable TIM2 update interrupt
    HAL_TIM_Base_Start_IT(&htim2);               // start TIM2
    HAL_TIM_Base_Start(&htim16);                 // start TIM16
}

static inline bool getHRTIMOutputState()
{
    return hhrtim1.Instance->sCommonRegs.OENR & (HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
}

static inline void HRTIMDisableOutput()
{
    hhrtim1.Instance->sCommonRegs.ODISR |= HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2;
    Status::status.outputEnabled = false;
}

static inline void HRTIMEnableOutput()
{
    /* enable HRTIM output */
    if (ControlData::controlData.enableOutput)
    {
        hhrtim1.Instance->sCommonRegs.OENR |= HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2;
        Status::status.outputEnabled = true;
    }
}

}  // namespace TimerManager

namespace SampleManager
{
ADCSample::ADCSample(uint16_t *_buffer, uint16_t _offset) : sum(4095U * ADC_SAMPLE_COUNT), buffer(_buffer), offset(_offset) {}
constexpr void ADCSample::sumBuffer()
{
    temp = 0;

    for (uint16_t i = offset; i < ADC_BUFFER_SIZE; i += ADC_CHANNEL_COUNT)
    {
        temp += buffer[i];
    }

    sum = temp;
}
ProcessedData::ProcessedData() : pReferee(0.0f), iReferee(0.0f), iChassis(0.0f), vASide(0.0f), iASide(0.0f), vBSide(0.0f), iBSide(0.0f) {}
ProcessedData ProcessedData::processedData;
void ProcessedData::updateAndCalibrate(const ADCSample &sampleVASide,
                                       const ADCSample &sampleVBSide,
                                       const ADCSample &sampleIReferee,
                                       const ADCSample &sampleIASide,
                                       const ADCSample &sampleIBSide)
{
    this->vASide = CALI_VA(sampleVASide.sum);
    this->vASide = M_MAX(this->vASide, 0.4f);
    this->vBSide = CALI_VB(sampleVBSide.sum);
    this->vBSide = M_MAX(this->vBSide, 0.4f);

    // I_Referee should have (RefereeSystem -> DCDC_ASide) as positive
    // I_ASide should have (DCDC_ASide->BSide) as positive

    this->iASide     = CALI_I_A_CONVERT(sampleIASide.sum) * ADC_FILTER_ALPHA + this->lastIASide * (1 - ADC_FILTER_ALPHA);
    this->lastIASide = this->iASide;

    this->iBSide     = CALI_I_B_CONVERT(sampleIBSide.sum) * ADC_FILTER_ALPHA + this->lastIBSide * (1 - ADC_FILTER_ALPHA);
    this->lastIBSide = this->iBSide;

    this->iReferee   = CALI_I_REFEREE(sampleIReferee.sum) * ADC_FILTER_ALPHA + this->lastIJudge * (1 - ADC_FILTER_ALPHA);
    this->lastIJudge = this->iReferee;

    this->iChassis = this->iReferee - this->iASide;

    this->pReferee = this->vASide * this->iReferee;

    float tmpPASide = this->vASide * this->iASide;
    this->pASide    = M_ABS(tmpPASide);

    float tmpPBSide = this->vBSide * this->iBSide;
    this->pBSide    = M_ABS(tmpPBSide);

    if (this->pASide > this->pBSide)
    {
        if (this->pBSide > 1.0f)
            this->efficiency = this->pBSide / this->pASide;
        else
            this->efficiency = 0.4f;
    }
    else
    {
        if (this->pASide > 1.0f)
            this->efficiency = this->pASide / this->pBSide;
        else
            this->efficiency = 0.4f;
    }

    this->capCharge += this->iBSide;
}

__attribute__((section(".BUFFER"))) static uint16_t adc1Buffer[ADC_BUFFER_SIZE] = {};
__attribute__((section(".BUFFER"))) static uint16_t adc2Buffer[ADC_BUFFER_SIZE] = {};

ADCSample adcVA(adc1Buffer, 3 - 1);
ADCSample adcIReferee(adc1Buffer, 2 - 1);
ADCSample adcIA(adc1Buffer, 1 - 1);
ADCSample adcVB(adc2Buffer, 2 - 1);
ADCSample adcIB(adc2Buffer, 1 - 1);
ADCSample adcNTC(adc2Buffer, 3 - 1);

static inline void ADCCalibrate()
{
    /* calibrate ADC */
    HAL_Delay(100);
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_Delay(100);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_Delay(100);
}

static inline void ADCDMAStart()
{
    /* start ADC */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc1Buffer, ADC_BUFFER_SIZE);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc2Buffer, ADC_BUFFER_SIZE);
}

}  // namespace SampleManager

// todo: PID parameter
IncrementalPID::IncrementalPID pidVoltageB(1.00f, 2.00f, 0.05f, 0.7f);
// PowerPID::PowerPID pidCurrentA(0.1f, 0.25f, 0.025f, 0.03f);

/*
 * With VA/VB feedforward:
 * PowerPID::PowerPID pidCurrentA(0.0056f, 0.0092f, 0.0012f, 0.002f);
 */

// Raw PID
// IncrementalPID::IncrementalPID pidCurrentA(0.0017f, 0.007f, 0.0016f, 0.0005f); //2024/5之前调出来的，似乎还是有点慢
IncrementalPID::IncrementalPID pidCurrentA(0.0046f, 0.0091f, 0.0015f, 0.0015f);  // 2024/5/15调的，大概5~10ms

IncrementalPID::IncrementalPID pidPowerReferee(0.205f, 0.31f, 0.03f, 0.022f);
IncrementalPID::IncrementalPID pidEnergyReferee(0.5f, 2.0f, 0.5f, 0.2f);

void systemRestart()
{
    TimerManager::HRTIMDisableOutput();
    __disable_irq();
    while (true)
        NVIC_SystemReset();
}

namespace ErrorChecker
{
struct ErrorCheckData
{
    uint16_t restartCoolDown        = 0;
    uint16_t errorCoolDown          = 0;
    uint16_t shortCircuitCnt        = 0;
    float lastCapVoltage            = 0.0f;
    float capVoltageChange          = 0.0f;
    uint16_t noCapCheckCnt          = 0;
    uint16_t buckBoostCheckCnt      = 0;
    uint16_t overVoltageCnt         = 0;
    uint8_t currentError            = 0;
    uint16_t capacitanceEstimateCnt = 0;
    int16_t capErrorCnt             = 0;
    uint16_t lowBatteryWarningCnt   = 0;
} errorCheckData;

bool checkHardwareError()
{
    struct HardwareErrorCheckData
    {
        float deltaVA = 0.0f;
        float deltaVB = 0.0f;
        float deltaIA = 0.0f;
        float deltaIB = 0.0f;
        float lastVA  = 0.0f;
        float lastVB  = 0.0f;
        float lastIA  = 0.0f;
        float lastIB  = 0.0f;
    } hardwareErrorCheckData;

    for (uint16_t i = 0; i < 10; i++)
        if (isnan(SampleManager::ProcessedData::processedData.pReferee) || isnan(SampleManager::ProcessedData::processedData.iReferee) ||
            isnan(SampleManager::ProcessedData::processedData.iChassis) || isnan(SampleManager::ProcessedData::processedData.vASide) ||
            isnan(SampleManager::ProcessedData::processedData.iASide) || isnan(SampleManager::ProcessedData::processedData.vBSide) ||
            isnan(SampleManager::ProcessedData::processedData.iBSide) || isnan(SampleManager::ProcessedData::processedData.pASide) ||
            isnan(SampleManager::ProcessedData::processedData.pBSide) || isnan(SampleManager::ProcessedData::processedData.efficiency) ||
            isnan(SampleManager::ProcessedData::processedData.temperature) || M_ABS(SampleManager::ProcessedData::processedData.iASide) > 1.0f ||
            M_ABS(SampleManager::ProcessedData::processedData.iBSide) > 1.0f || SampleManager::ProcessedData::processedData.iReferee < -1.0f ||
            SampleManager::ProcessedData::processedData.iReferee > 8.0f || SampleManager::adcNTC.sum == 4095U * ADC_SAMPLE_COUNT ||
            SampleManager::adcIA.sum == 4095U * ADC_SAMPLE_COUNT || SampleManager::adcIB.sum == 4095U * ADC_SAMPLE_COUNT ||
            SampleManager::adcIReferee.sum == 4095U * ADC_SAMPLE_COUNT || SampleManager::adcVA.sum == 4095U * ADC_SAMPLE_COUNT ||
            SampleManager::adcVB.sum == 4095U * ADC_SAMPLE_COUNT)
            while (true)
            {
                Status::status.errorCode |= ERROR_BUCK_BOOST;
                TimerManager::HRTIMDisableOutput();
                Buzzer::play(2700, 1000);
                HAL_Delay(2000);

                return false;
            }

    hardwareErrorCheckData.lastVA = SampleManager::ProcessedData::processedData.vASide;
    hardwareErrorCheckData.lastVB = SampleManager::ProcessedData::processedData.vBSide;
    hardwareErrorCheckData.lastIA = SampleManager::ProcessedData::processedData.iASide;
    hardwareErrorCheckData.lastIB = SampleManager::ProcessedData::processedData.iBSide;
    HAL_Delay(1);

    for (int i = 0; i < 100; i++)
    {
        hardwareErrorCheckData.deltaIA += M_ABS(SampleManager::ProcessedData::processedData.iASide - hardwareErrorCheckData.lastIA);
        hardwareErrorCheckData.deltaIB += M_ABS(SampleManager::ProcessedData::processedData.iBSide - hardwareErrorCheckData.lastIB);
        hardwareErrorCheckData.deltaVA += M_ABS(SampleManager::ProcessedData::processedData.vASide - hardwareErrorCheckData.lastVA);
        hardwareErrorCheckData.deltaVB += M_ABS(SampleManager::ProcessedData::processedData.vBSide - hardwareErrorCheckData.lastVB);

        hardwareErrorCheckData.lastVA = SampleManager::ProcessedData::processedData.vASide;
        hardwareErrorCheckData.lastVB = SampleManager::ProcessedData::processedData.vBSide;
        hardwareErrorCheckData.lastIA = SampleManager::ProcessedData::processedData.iASide;
        hardwareErrorCheckData.lastIB = SampleManager::ProcessedData::processedData.iBSide;

        HAL_Delay(1);
    }

    if (hardwareErrorCheckData.deltaIA > 500.0f || hardwareErrorCheckData.deltaIB > 500.0f || hardwareErrorCheckData.deltaVA > 100000.0f ||
        hardwareErrorCheckData.deltaVB > 100000.0f)
    {
        while (true)
        {
            Status::status.errorCode |= ERROR_BUCK_BOOST;
            TimerManager::HRTIMDisableOutput();
            Buzzer::play(2700, 1000);
            HAL_Delay(2000);

            return false;
        }
    }

    errorCheckData.restartCoolDown   = 0;
    errorCheckData.errorCoolDown     = 0;
    errorCheckData.shortCircuitCnt   = 0;
    errorCheckData.lastCapVoltage    = SampleManager::ProcessedData::processedData.vBSide;
    errorCheckData.noCapCheckCnt     = 0;
    errorCheckData.buckBoostCheckCnt = 0;
    errorCheckData.overVoltageCnt    = 0;
    errorCheckData.currentError      = 0;
    return true;
}
static void handleShortCircuit()
{
#ifndef CALIBRATION_MODE
    if (SampleManager::ProcessedData::processedData.vASide < SHORT_CIRCUIT_VOLTAGE)
    {
        if (M_ABS(SampleManager::ProcessedData::processedData.iASide) > SHORT_CIRCUIT_CURRENT)
        {
            errorCheckData.currentError |= ERROR_SHORT_CIRCUIT;
            Status::status.errorCode |= ERROR_SHORT_CIRCUIT;
            if (TimerManager::getHRTIMOutputState() == true)
            {
                if (errorCheckData.shortCircuitCnt++ >= 5)
                {
                    errorCheckData.shortCircuitCnt        = 5;
                    ControlData::controlData.enableOutput = false;
                }
                TimerManager::HRTIMDisableOutput();
                errorCheckData.restartCoolDown = 500;
                errorCheckData.errorCoolDown   = 500;
                Buzzer::play(200, 100);
            }
            errorCheckData.restartCoolDown = 200;
        }
    }
    if (SampleManager::ProcessedData::processedData.vBSide < SHORT_CIRCUIT_VOLTAGE)
    {
        if (M_ABS(SampleManager::ProcessedData::processedData.iBSide) > SHORT_CIRCUIT_CURRENT)
        {
            errorCheckData.currentError |= ERROR_SHORT_CIRCUIT;
            Status::status.errorCode |= ERROR_SHORT_CIRCUIT;
            if (TimerManager::getHRTIMOutputState() == true)
            {
                if (errorCheckData.shortCircuitCnt++ > 5)
                {
                    errorCheckData.shortCircuitCnt        = 5;
                    ControlData::controlData.enableOutput = false;
                }
                errorCheckData.restartCoolDown = 500;
                errorCheckData.errorCoolDown   = 500;
                TimerManager::HRTIMDisableOutput();
                Buzzer::play(200, 100);
            }
            errorCheckData.restartCoolDown = 200;
        }
    }
#endif
}
static void handleErrorState()
{
    /* Under Voltage */
#ifdef CALIBRATION_MODE
    if (SampleManager::ProcessedData::processedData.vASide < 12.0f && SampleManager::ProcessedData::processedData.vBSide < 12.0f)
    {
        ControlData::controlData.enableOutput = false;
        errorCheckData.currentError |= ERROR_UNDER_VOLTAGE;
        Status::status.errorCode |= ERROR_UNDER_VOLTAGE;
    }
    else
    {
        errorCheckData.currentError &= ~ERROR_UNDER_VOLTAGE;
        Status::status.errorCode &= ~ERROR_UNDER_VOLTAGE;
    }
#else
    if (SampleManager::ProcessedData::processedData.vASide < 15.0f)
    {
        errorCheckData.currentError |= ERROR_NO_POWER_INPUT;
        Status::status.errorCode |= ERROR_NO_POWER_INPUT;
        if (SampleManager::ProcessedData::processedData.vBSide < 12.0f)
        {
            errorCheckData.currentError |= ERROR_UNDER_VOLTAGE;
            Status::status.errorCode |= ERROR_UNDER_VOLTAGE;
        }
        else
        {
            errorCheckData.currentError &= ~ERROR_UNDER_VOLTAGE;
            Status::status.errorCode &= ~ERROR_UNDER_VOLTAGE;
        }
        Status::status.lowBattery = false;
    }
    else
    {
        errorCheckData.currentError &= ~(ERROR_NO_POWER_INPUT | ERROR_UNDER_VOLTAGE);
        Status::status.errorCode &= ~(ERROR_NO_POWER_INPUT | ERROR_UNDER_VOLTAGE);
        if (Status::status.lowBattery == true)
        {
            if (++errorCheckData.lowBatteryWarningCnt >= 7000)
            {
                errorCheckData.lowBatteryWarningCnt = 0;
                Buzzer::play(3200, 1600);
            }

            if (SampleManager::ProcessedData::processedData.vASide > LOW_BATTERY_RECOVER_VOLTAGE)
                Status::status.lowBattery = false;
        }
        else
        {
            if (SampleManager::ProcessedData::processedData.vASide < LOW_BATTERY_VOLTAGE)
            {
                Status::status.lowBattery           = true;
                errorCheckData.lowBatteryWarningCnt = 0;
            }
        }
    }
#endif

    /* Over Voltage */
    if (SampleManager::ProcessedData::processedData.vASide > 31.0f || SampleManager::ProcessedData::processedData.vBSide > 31.0f)
    {
        errorCheckData.currentError |= ERROR_OVER_VOLTAGE;
        Status::status.errorCode |= ERROR_OVER_VOLTAGE;
    }
    else
    {
        if (SampleManager::ProcessedData::processedData.vASide > 27.0f)
            errorCheckData.overVoltageCnt += 1;
        else if (SampleManager::ProcessedData::processedData.vASide > 28.0f)
            errorCheckData.overVoltageCnt += 5;
        else if (SampleManager::ProcessedData::processedData.vASide > 29.0f)
            errorCheckData.overVoltageCnt += 25;
        else if (SampleManager::ProcessedData::processedData.vASide > 30.0f)
            errorCheckData.overVoltageCnt += 125;
        else
            errorCheckData.overVoltageCnt = 0;

        if (errorCheckData.overVoltageCnt >= 300)
        {
            errorCheckData.overVoltageCnt = 300;
            errorCheckData.currentError |= ERROR_OVER_VOLTAGE;
            Status::status.errorCode |= ERROR_OVER_VOLTAGE;
        }
        else
        {
            errorCheckData.currentError &= ~ERROR_OVER_VOLTAGE;
        }
    }

    /**
     * @brief Short Circuit Check
     *
     * @note When short circuit occurred, the output voltage will be pull down to near 0V, and the output current will be very large.
     */
    if (!((SampleManager::ProcessedData::processedData.vASide < SHORT_CIRCUIT_VOLTAGE &&
           M_ABS(SampleManager::ProcessedData::processedData.iASide) > SHORT_CIRCUIT_CURRENT)) &&
        !((SampleManager::ProcessedData::processedData.vBSide < SHORT_CIRCUIT_VOLTAGE &&
           M_ABS(SampleManager::ProcessedData::processedData.iBSide) > SHORT_CIRCUIT_CURRENT)))
        errorCheckData.currentError &= ~ERROR_SHORT_CIRCUIT;

    /* Over Temperature */
    if (errorCheckData.currentError & ERROR_HIGH_TEMPERATURE)
    {
        if (SampleManager::ProcessedData::processedData.temperature > OVER_TEMP_RECOVER_THRESHOLD)
        {
            errorCheckData.currentError &= ~ERROR_HIGH_TEMPERATURE;
            Status::status.errorCode &= ~ERROR_HIGH_TEMPERATURE;
        }
    }
    else
    {
        if (SampleManager::ProcessedData::processedData.temperature < OVER_TEMP_TRIGGER_THRESHOLD)
        {
            errorCheckData.currentError |= ERROR_HIGH_TEMPERATURE;
            Status::status.errorCode |= ERROR_HIGH_TEMPERATURE;
        }
    }

#ifndef CALIBRATION_MODE
#ifndef IGNORE_CAPACITOR_ERROR
    /**
     * @brief Capacitor Error check
     *
     * @note When the capacitor is disconnected, the voltage on the capacitor will change very fast, and the resultant capacitance will be very
     * small. The capacitor voltage will be monitored, and the capacitance will be estimated by the voltage change and charge goes into / out the
     * capacitor. If the estimated capacitance is too small, the capacitor is considered to be disconnected.
     *
     * @note When the capacitor is shorted or overcharged, the voltage on the capacitor will change very slow, and the charge goes into / out the
     * capacitor will be very large. The charge will be monitored, if the charge is too large, the capacitor is considered to be shorted or
     * overcharged.
     */
    errorCheckData.capVoltageChange += ProcessedSampleData::processedSampleData.vBSide - errorCheckData.lastCapVoltage;
    errorCheckData.lastCapVoltage = ProcessedSampleData::processedSampleData.vBSide;
    if (errorCheckData.capVoltageChange > 3.0f || errorCheckData.capVoltageChange < -3.0f)
    {
        float estimatedCapacitance = ProcessedSampleData::processedSampleData.capCharge / errorCheckData.capVoltageChange * 2.777777809e-05f;
        estimatedCapacitance       = M_ABS(estimatedCapacitance);

        if (Status::status.outputEnabled && estimatedCapacitance < 4e-5f)  // cap disconnected
        {
            errorCheckData.currentError |= ERROR_CAPACITOR;
            Status::status.errorCode |= ERROR_CAPACITOR;
            ControlData::controlData.enableOutput = false;
        }

        errorCheckData.capacitanceEstimateCnt              = 0;
        errorCheckData.capVoltageChange                    = 0.0f;
        ProcessedSampleData::processedSampleData.capCharge = 0.0f;
    }
    else if (ProcessedSampleData::processedSampleData.capCharge > 200000.0f || ProcessedSampleData::processedSampleData.capCharge < -200000.0f)
    {
        if (Status::status.outputEnabled && errorCheckData.capVoltageChange < 0.2f &&
            errorCheckData.capVoltageChange > -0.2f)  // cap overcharged / short circuit
        {
            errorCheckData.capErrorCnt += 1000;
            if (errorCheckData.capErrorCnt > 5000)
            {
                errorCheckData.capErrorCnt = 0;
                errorCheckData.currentError |= ERROR_CAPACITOR;
                Status::status.errorCode |= ERROR_CAPACITOR;
                ControlData::controlData.enableOutput = false;
            }
        }
        errorCheckData.capacitanceEstimateCnt              = 0;
        errorCheckData.capVoltageChange                    = 0.0f;
        ProcessedSampleData::processedSampleData.capCharge = 0.0f;
    }
    else if (errorCheckData.capacitanceEstimateCnt++ > 10000)
    {
        errorCheckData.capacitanceEstimateCnt              = 0;
        errorCheckData.capVoltageChange                    = 0.0f;
        ProcessedSampleData::processedSampleData.capCharge = 0.0f;
    }

    if (--errorCheckData.capErrorCnt < 0)
        errorCheckData.capErrorCnt = 0;

    if (ControlData::controlData.enableOutput == true)
        errorCheckData.currentError &= ~ERROR_CAPACITOR;
#endif

    /**
     * @brief Buck-Boost Error Check
     *
     * @note When error with the buck-boost circuit occurred, normally, the efficiency will be very low, and the duty ratio will be abnormal.
     * This is sometimes caused by the burned MOSFET, abnormal MOSFET driver, or abnormal MOSFET driving voltage.
     */
    float dutyDiffRatio =
        SampleManager::ProcessedData::processedData.vASide / SampleManager::ProcessedData::processedData.vBSide * tempData.outputDuty;
    dutyDiffRatio = M_ABS(dutyDiffRatio);
    if (Status::status.outputEnabled &&
        (((SampleManager::ProcessedData::processedData.pASide > 10.0f || SampleManager::ProcessedData::processedData.pBSide > 10.0f) &&
          SampleManager::ProcessedData::processedData.efficiency < 0.5f) ||  // efficiency abnormal
         dutyDiffRatio > 2.0f ||
         dutyDiffRatio < 0.5f))  // duty ratio abnormal
    {
        if (errorCheckData.buckBoostCheckCnt++ > 40)
        {
            errorCheckData.currentError |= ERROR_BUCK_BOOST;
            Status::status.errorCode |= ERROR_BUCK_BOOST;
            ControlData::controlData.enableOutput = false;
            errorCheckData.buckBoostCheckCnt      = 0;
        }
    }
    else
    {
        errorCheckData.buckBoostCheckCnt = 0;
        if (ControlData::controlData.enableOutput == true)
            errorCheckData.currentError &= ~ERROR_BUCK_BOOST;
    }

#endif

    if (errorCheckData.currentError != 0)
    {
        if (Status::status.outputEnabled == true)
        {
            TimerManager::HRTIMDisableOutput();
            Buzzer::play(200, 200);
        }
        errorCheckData.restartCoolDown = 500;
        errorCheckData.errorCoolDown   = 500;
    }
    else
    {
        if (errorCheckData.restartCoolDown > 0)
            errorCheckData.restartCoolDown--;
        else
        {
            if (ControlData::controlData.enableOutput != false)
            {
                if (Status::status.outputEnabled == false)
                {
                    TimerManager::HRTIMEnableOutput();
                    Buzzer::play(1250, 200);
                }

                if (errorCheckData.errorCoolDown > 0)
                    errorCheckData.errorCoolDown--;
                else
                {
                    Status::status.errorCode       = 0;
                    errorCheckData.shortCircuitCnt = 0;
                }
            }
            else
            {
                if (Status::status.outputEnabled == true)
                {
                    TimerManager::HRTIMDisableOutput();
                    Buzzer::play(200, 200);
                }
            }
        }
    }
}
}  // namespace ErrorChecker

void init()
{
powerManagerInit:

    TimerManager::HRTIMDisableOutput();

    ControlData::controlData.enableOutput = false;

    SampleManager::ADCCalibrate();
    SampleManager::ADCDMAStart();
    TimerManager::HRTIMConfig();
    TimerManager::HRTIMStartPWM();

    /* wait for ADC DMA */
    TimerManager::TIMConfig();

    HAL_Delay(100);

#ifndef CALIBRATION_MODE

    if (!ErrorChecker::checkHardwareError())
        goto powerManagerInit;

    ControlData::controlData.enableOutput = true;
#endif
}

static inline void updatePWM(float VBToVA);

struct PWMDutyVariable
{
    bool buckBoostMode = false;
    float dutyA        = 0.0f;
    float dutyB        = 0.0f;
    uint16_t ACMP1     = 8000;
    uint16_t ACMP3     = 8000;
    uint16_t BCMP1     = 8000;
    uint16_t BCMP3     = 8000;
};

static PWMDutyVariable pwmDutyVariable;

static inline void updatePWM(float VBToVA)
{
    VBToVA = M_CLAMP(VBToVA, 0.05f, 10.0f);

    if (pwmDutyVariable.buckBoostMode)
    {
        if (VBToVA < 0.8f || VBToVA > 1.25f)
            pwmDutyVariable.buckBoostMode = false;
    }
    else
    {
        if (VBToVA > 0.9f && VBToVA < 1.111f)
            pwmDutyVariable.buckBoostMode = true;
    }

    if (pwmDutyVariable.buckBoostMode)
    {
        pwmDutyVariable.dutyA = (VBToVA + 1.0f) * 0.4f;
        pwmDutyVariable.dutyB = (1.0f / VBToVA + 1.0f) * 0.4f;
    }
    else
    {
        if (VBToVA < 1)
        {
            pwmDutyVariable.dutyA = 0.9f * VBToVA;
            pwmDutyVariable.dutyB = 0.9f;
        }
        else
        {
            pwmDutyVariable.dutyA = 0.9f;
            pwmDutyVariable.dutyB = 0.9f / VBToVA;
        }
    }

    pwmDutyVariable.ACMP1 = (HRTIM_PERIOD / 2) * (1.0f - pwmDutyVariable.dutyA);
    pwmDutyVariable.ACMP3 = (HRTIM_PERIOD / 2) * (1.0f + pwmDutyVariable.dutyA);
    pwmDutyVariable.BCMP1 = (HRTIM_PERIOD / 2) * (1.0f - pwmDutyVariable.dutyB);
    pwmDutyVariable.BCMP3 = (HRTIM_PERIOD / 2) * (1.0f + pwmDutyVariable.dutyB);

    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = pwmDutyVariable.ACMP1;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP3xR = pwmDutyVariable.ACMP3;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP1xR = pwmDutyVariable.BCMP1;
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP3xR = pwmDutyVariable.BCMP3;
}

static inline void updateVIP()
{
#ifndef CALIBRATION_MODE
    if (Status::status.outputEnabled)  //[[likely]]
    {
        float tempVASide = SampleManager::ProcessedData::processedData.vASide;

        float absIA        = M_MIN(M_ABS(SampleManager::ProcessedData::processedData.iASide), 0.1f);
        float absIB        = M_MIN(M_ABS(SampleManager::ProcessedData::processedData.iBSide), 0.1f);
        float actualIAtoIB = absIA / absIB;

        pidPowerReferee.update(tempData.targetRefereePower, SampleManager::ProcessedData::processedData.pReferee);

        tempData.targetAPower += pidPowerReferee.getDeltaOutput();

        float tempCapOutILimit;
        float tempCapInILimit        = I_LIMIT;
        constexpr float MIN_CAP_IOUT = 0.1f;
        constexpr float MAX_CAP_IOUT = I_LIMIT;
        constexpr float CAP_V_CUTOFF = 5.0f;
        constexpr float CAP_V_NORMAL = 12.0f;
        if (SampleManager::ProcessedData::processedData.vBSide < CAP_V_CUTOFF)
        {
            tempCapOutILimit = MIN_CAP_IOUT;
            tempCapInILimit  = 4.0f;
        }
        else if (SampleManager::ProcessedData::processedData.vBSide > CAP_V_NORMAL)
        {
            tempCapOutILimit = MAX_CAP_IOUT;
        }
        else
        {
            tempCapOutILimit = MIN_CAP_IOUT + (MAX_CAP_IOUT - MIN_CAP_IOUT) * (SampleManager::ProcessedData::processedData.vBSide - CAP_V_CUTOFF) /
                                                  (CAP_V_NORMAL - CAP_V_CUTOFF);
            tempCapOutILimit = M_CLAMP(tempCapOutILimit, MIN_CAP_IOUT, MAX_CAP_IOUT);
        }

        float powerALimitNegativeBoundByA = -1 * I_LIMIT * tempVASide;
        float powerALimitPositiveBoundByA = I_LIMIT * tempVASide;

        float powerALimitNegativeBoundByB = -1 * tempCapOutILimit * tempVASide * actualIAtoIB;
        float powerALimitPositiveBoundByB = tempCapInILimit * tempVASide * actualIAtoIB;

        float powerLimitAToB = M_MIN(powerALimitPositiveBoundByA, powerALimitPositiveBoundByB);
        float powerLimitBToA = M_MAX(powerALimitNegativeBoundByA, powerALimitNegativeBoundByB);

        if (tempData.targetAPower < powerLimitBToA)
        {
            tempData.targetAPower = powerLimitBToA;
            if (tempData.baseRefereePower > ControlData::controlData.refereePowerLimit + 3.0f)
                tempData.baseRefereePower = ControlData::controlData.refereePowerLimit + 3.0f;
        }
        else if (tempData.targetAPower > powerLimitAToB)
        {
            tempData.targetAPower = powerLimitAToB;
            if (tempData.baseRefereePower > ControlData::controlData.refereePowerLimit + 3.0f)
                tempData.baseRefereePower = ControlData::controlData.refereePowerLimit + 3.0f;
        }

        tempData.targetIA = tempData.targetAPower / tempVASide;

        pidCurrentA.update(tempData.targetIA, SampleManager::ProcessedData::processedData.iASide);

        static float extraErrorSum = 0;
        static float kII           = 1.1e-05f;
        //        static volatile float kII = 0;
        static float iiDecay = 0.988f;
        if (kII != 0)
        {
            extraErrorSum = extraErrorSum * iiDecay + (tempData.targetIA - SampleManager::ProcessedData::processedData.iASide);
            extraErrorSum = M_CLAMP(extraErrorSum, -500, 500);
        }

        float iaDuty = tempData.outputDuty + (pidCurrentA.getDeltaOutput() + kII * extraErrorSum);

        if (SampleManager::ProcessedData::processedData.vBSide > MAX_CAP_VOLTAGE * 0.9f)
        {
            pidVoltageB.update(MAX_CAP_VOLTAGE, SampleManager::ProcessedData::processedData.vBSide);

            static volatile float vbDuty = 1;
            vbDuty                       = (tempData.outputDuty * SampleManager::ProcessedData::processedData.vASide + pidVoltageB.getDeltaOutput()) /
                     SampleManager::ProcessedData::processedData.vASide;
            if (vbDuty < iaDuty)
            {
                tempData.outputDuty = vbDuty;
                extraErrorSum       = 0;
                if (tempData.baseRefereePower > ControlData::controlData.refereePowerLimit + 3.0f)
                    tempData.baseRefereePower = ControlData::controlData.refereePowerLimit + 3.0f;
                if (tempData.targetAPower > SampleManager::ProcessedData::processedData.iBSide * MAX_CAP_VOLTAGE + 8.0f)
                    tempData.targetAPower = SampleManager::ProcessedData::processedData.iBSide * MAX_CAP_VOLTAGE + 8.0f;
            }
            else
                tempData.outputDuty = iaDuty;
        }
        else
        {
            pidVoltageB.updateDataNoOutput(MAX_CAP_VOLTAGE, SampleManager::ProcessedData::processedData.vBSide);
            tempData.outputDuty = iaDuty;
        }

        tempData.outputDuty = M_CLAMP(tempData.outputDuty, 0.05f, 10.0f);
    }
    else
    {
        tempData.outputDuty   = SampleManager::ProcessedData::processedData.vBSide / SampleManager::ProcessedData::processedData.vASide;
        tempData.targetAPower = 0.0f;
        tempData.targetIA     = 0.0f;
        tempData.outputDuty   = M_CLAMP(tempData.outputDuty, 0.05f, 10.0f);

        pidCurrentA.updateDataNoOutput(0, SampleManager::ProcessedData::processedData.iASide);
        pidVoltageB.updateDataNoOutput(MAX_CAP_VOLTAGE, SampleManager::ProcessedData::processedData.vBSide);
        pidPowerReferee.updateDataNoOutput(tempData.targetRefereePower, SampleManager::ProcessedData::processedData.pReferee);
    }
#endif
    updatePWM(tempData.outputDuty);
}

static inline void updateStatus()
{
    Status::status.realVBToVA = SampleManager::ProcessedData::processedData.vBSide / SampleManager::ProcessedData::processedData.vASide;

    Status::status.outputEnabled = TimerManager::getHRTIMOutputState();

    Status::status.capEnergy =
        SampleManager::ProcessedData::processedData.vBSide * SampleManager::ProcessedData::processedData.vBSide / (MAX_CAP_VOLTAGE * MAX_CAP_VOLTAGE);
    Status::status.chassisPower = -SampleManager::ProcessedData::processedData.iASide * SampleManager::ProcessedData::processedData.vASide +
                                  SampleManager::ProcessedData::processedData.pReferee;

    if (SampleManager::ProcessedData::processedData.vBSide > SampleManager::ProcessedData::processedData.vASide)
        Status::status.chassisPowerLimit = tempData.baseRefereePower + SampleManager::ProcessedData::processedData.vASide * I_LIMIT * 0.8f;
    else
        Status::status.chassisPowerLimit = tempData.baseRefereePower + SampleManager::ProcessedData::processedData.vBSide * I_LIMIT * 0.8f;

    if (Status::status.communicationTimeoutCnt++ >= 500)
    {
        ControlData::controlData.energyRemain      = ENERGY_BUFFER;
        ControlData::controlData.refereePowerLimit = DEFAULT_REFEREE_POWER;
        tempData.baseRefereePower                  = DEFAULT_REFEREE_POWER;
        tempData.targetRefereePower                = DEFAULT_REFEREE_POWER;
        Status::status.communicationTimeoutCnt     = 500;
        pidEnergyReferee.reset();
    }

    SampleManager::adcNTC.sumBuffer();
    SampleManager::ProcessedData::processedData.temperature = SampleManager::adcNTC.sum * DIVIDED_BY_ADC_SAMPLE_COUNT;

    if (tempData.ledBlinkCnt++ >= 200)
    {
        tempData.ledBlinkCnt = 0;
    }

    if (Status::status.errorCode != 0)
    {
        if (tempData.ledBlinkCnt >= 100)
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    }

    if (ControlData::controlData.enableOutput == true)
    {
        if (Status::status.outputEnabled == false)
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        else
        {
            if (tempData.ledBlinkCnt >= 100)
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
            else
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        }
    }
    else
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
}

void updateEnergy()
{
    if (tempData.lastRefereePowerLimit != ControlData::controlData.refereePowerLimit)
    {
        tempData.lastRefereePowerLimit = ControlData::controlData.refereePowerLimit;
        tempData.baseRefereePower += ControlData::controlData.refereePowerLimit - tempData.targetRefereePower;
        pidPowerReferee.reset();
    }
    if (ControlData::controlData.energyRemain > ENERGY_BUFFER / 2.0f)
    {
        if (Status::status.communicationTimeoutCnt < 80)
            return;
        PowerManager::Status::status.communicationTimeoutCnt = 0;
        pidEnergyReferee.update(ENERGY_BUFFER, ControlData::controlData.energyRemain);
        tempData.baseRefereePower -= pidEnergyReferee.getDeltaOutput();
        tempData.baseRefereePower = M_CLAMP(tempData.baseRefereePower, 10.0f, ControlData::controlData.refereePowerLimit + 30.0f);
    }
    else
    {
        PowerManager::Status::status.communicationTimeoutCnt = 0;
        pidEnergyReferee.updateDataNoOutput(ENERGY_BUFFER, ControlData::controlData.energyRemain);
        tempData.baseRefereePower = 10.0f;
    }
    tempData.targetRefereePower = tempData.baseRefereePower;
}

static volatile bool blocking = false;

/**
 * Testing Functions, mainly for PID parameter testing.
 * */
namespace Test
{

static volatile float testTargetIA = 2.5f;
static inline void updateVITest()
{
    if (Status::status.outputEnabled)
    {
        pidCurrentA.update(testTargetIA, SampleManager::ProcessedData::processedData.iASide);

        static float extraErrorSum1  = 0;
        static volatile float kII1   = 1.1e-05f;
        static volatile float decay1 = 0.988f;
        if (kII1 != 0)
        {
            extraErrorSum1 = extraErrorSum1 * decay1 + (testTargetIA - SampleManager::ProcessedData::processedData.iASide);
            extraErrorSum1 = M_CLAMP(extraErrorSum1, -500, 500);
        }

        float iaDuty = tempData.outputDuty + (pidCurrentA.getDeltaOutput() + kII1 * extraErrorSum1);

        if (SampleManager::ProcessedData::processedData.vBSide > MAX_CAP_VOLTAGE * 0.90f)
        {
            pidVoltageB.update(MAX_CAP_VOLTAGE, SampleManager::ProcessedData::processedData.vBSide);
            float vbDuty = (tempData.outputDuty * SampleManager::ProcessedData::processedData.vASide + pidVoltageB.getDeltaOutput()) /
                           SampleManager::ProcessedData::processedData.vASide;
            if (vbDuty < iaDuty)
            {
                tempData.outputDuty = vbDuty;
                extraErrorSum1      = 0;
            }
            else
            {
                tempData.outputDuty = iaDuty;
            }
        }
        else
        {
            pidVoltageB.updateDataNoOutput(MAX_CAP_VOLTAGE, SampleManager::ProcessedData::processedData.vBSide);
            tempData.outputDuty = iaDuty;
        }

        tempData.outputDuty = M_CLAMP(tempData.outputDuty, 0.05f, 10.0f);
        updatePWM(tempData.outputDuty);
    }
    else
    {
        tempData.outputDuty = SampleManager::ProcessedData::processedData.vBSide / SampleManager::ProcessedData::processedData.vASide;
        tempData.outputDuty = M_CLAMP(tempData.outputDuty, 0.05f, 10.0f);

        pidCurrentA.updateDataNoOutput(0, SampleManager::ProcessedData::processedData.iASide);
        pidVoltageB.updateDataNoOutput(MAX_CAP_VOLTAGE, SampleManager::ProcessedData::processedData.vBSide);
    }
    updatePWM(tempData.outputDuty);
}

volatile bool phas    = true;
volatile float testI1 = 1.5f;
volatile float testI2 = 2.5f;
void testICycle()
{
    static int tickITest = 0;
    tickITest++;
    if (tickITest > 3000)
    {
        tickITest    = 0;
        phas         = !phas;
        testTargetIA = phas ? testI1 : testI2;
    }
}

}  // namespace Test

/**
 * @brief call back functions
 */
extern "C"
{
    void HRTIM1_Master_IRQHandler(void)  // V I loop
    {
        __HAL_HRTIM_MASTER_CLEAR_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);

        float cnt = __HAL_TIM_GET_COUNTER(&htim16);
        __HAL_TIM_SET_COUNTER(&htim16, 0);

        SampleManager::adcVA.sumBuffer();
        SampleManager::adcIReferee.sumBuffer();
        SampleManager::adcIA.sumBuffer();
        SampleManager::adcVB.sumBuffer();
        SampleManager::adcIB.sumBuffer();
        SampleManager::adcNTC.sumBuffer();

        SampleManager::ProcessedData::processedData.updateAndCalibrate(
            SampleManager::adcVA, SampleManager::adcVB, SampleManager::adcIReferee, SampleManager::adcIA, SampleManager::adcIB);

        ErrorChecker::handleShortCircuit();

        updateVIP();

        if (__HAL_HRTIM_MASTER_GET_FLAG(&hhrtim1, HRTIM_MASTER_FLAG_MREP) != RESET)  // blocking detected
        {
            blocking = true;
            __HAL_HRTIM_MASTER_CLEAR_IT(&hhrtim1, HRTIM_MASTER_IT_MREP);  // stall the loop
        }

        static volatile float load;
        load = __HAL_TIM_GET_COUNTER(&htim16) / cnt;
    }

    void TIM2_IRQHandler(void)
    {
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
        updateStatus();
        ErrorChecker::handleErrorState();
        Communication::feedbackPowerData();
        // updateEnergyThief();
        // testICycle();
    }

}  // extern "C"

}  // namespace PowerManager