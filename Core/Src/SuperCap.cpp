/**
 * @file SuperCap.cpp
 * @author JIANG Yicheng RM2023
 * @brief
 * @version 0.1
 * @date 2023-01-22
 *
 * @copyright Copyright (c) 2023
 */

#include "Buzzer.hpp"
#include "Calibration.hpp"
#include "Communication.hpp"
#include "Config.hpp"
#include "PowerManager.hpp"
#include "iwdg.h"
#include "main.h"

namespace SuperCap
{
static void loop()
{
    while (true)
    {
        __HAL_IWDG_RELOAD_COUNTER(&hiwdg);
        HAL_Delay(1);
    }
}

void configIWDG()
{
    hiwdg.Instance = IWDG;
    __HAL_IWDG_START(&hiwdg);
    IWDG_ENABLE_WRITE_ACCESS(&hiwdg);
    hiwdg.Instance->PR  = IWDG_PRESCALER_4;
    hiwdg.Instance->RLR = 1000;
    while ((hiwdg.Instance->SR & (IWDG_SR_WVU | IWDG_SR_RVU | IWDG_SR_PVU)) != 0x00u)
        ;
    __HAL_IWDG_RELOAD_COUNTER(&hiwdg);
}

void init()
{
    HAL_Delay(200);  // wait voltage stabilization

    /* initialize communication */
    Buzzer::init();

    /* check hardware id */
    static volatile uint32_t hardwareID = 0;

    hardwareID = HAL_FLASHEx_OBGetUserData(OB_DATA_ADDRESS_DATA0);

    if (hardwareID != (uint32_t)HARDWARE_ID)
    {
#ifdef CALIBRATION_MODE
        while (HAL_FLASH_OB_Unlock() != HAL_OK)  // unlock flash option bytes
            __ASM volatile("bkpt ");

        while (HAL_FLASHEx_OBErase() != HAL_OK)  // erase option bytes
            __ASM volatile("bkpt ");

        FLASH_OBProgramInitTypeDef pOBInit;
        pOBInit.OptionType  = OPTIONBYTE_WRP | OPTIONBYTE_RDP | OPTIONBYTE_USER | OPTIONBYTE_DATA;
        pOBInit.WRPState    = OB_WRPSTATE_DISABLE;
        pOBInit.WRPPage     = 0xFFFFFFFF;
        pOBInit.RDPLevel    = OB_RDP_LEVEL_0;
        pOBInit.USERConfig  = OB_IWDG_SW | OB_STOP_NO_RST | OB_STDBY_NO_RST | OB_BOOT1_RESET | OB_VDDA_ANALOG_ON | OB_SRAM_PARITY_RESET;
        pOBInit.DATAAddress = OB_DATA_ADDRESS_DATA0;
        pOBInit.DATAData    = (uint8_t)HARDWARE_ID;        // set hardware id

        while (HAL_FLASHEx_OBProgram(&pOBInit) != HAL_OK)  // program option bytes
            __ASM volatile("bkpt ");

        while (HAL_FLASH_OB_Lock() != HAL_OK)  // lock flash option bytes
            __ASM volatile("bkpt ");

        while (HAL_FLASH_OB_Launch() != HAL_OK)  // launch option bytes
            __ASM volatile("bkpt ");

#endif
        Buzzer::play(1000);
        while (true)
            __ASM volatile("bkpt ");
    }

    /* initialize power manager */
    Buzzer::play(800, 500);

    Communication::init();

    HAL_Delay(600);

    PowerManager::init();

    configIWDG();
}

extern "C"
{
    void systemStart()
    {
        SuperCap::init();
        loop();
    }
}
}  // namespace SuperCap