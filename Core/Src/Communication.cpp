#include "Communication.hpp"

#include "MathUtil.hpp"
#include "PowerManager.hpp"
#include "can.h"

namespace Communication
{
struct RxData
{
    uint8_t enableDCDC : 1;
    uint8_t systemRestart : 1;
    uint8_t resv0 : 6;
    uint16_t feedbackRefereePowerLimit;
    uint16_t feedbackRefereeEnergyBuffer;
    uint8_t resv1[3];
} __attribute__((packed));

static RxData rxData;

struct TxData
{
    uint8_t errorCode;
    float chassisPower;
    uint16_t chassisPowerLimit;
    uint8_t capEnergy;
} __attribute__((packed));

static TxData txData;

void init()
{
    static_assert(sizeof(RxData) == 8, "RxData size error");
    static_assert(sizeof(TxData) == 8, "TxData size error");

    CAN_FilterTypeDef filter = {
        0x061 << 5,             // filterID HI
        0,                      // filterID LO
        0,                      // filterMask HI
        0,                      // filterMask LO
        CAN_RX_FIFO0,           // FIFO assignment
        CAN_FILTER_FIFO0,       // filterBank number
        CAN_FILTERMODE_IDLIST,  // filter mode
        CAN_FILTERSCALE_16BIT,  // filter size
        CAN_FILTER_ENABLE,      // ENABLE or DISABLE
        0                       // Slave start bank, STM32F334 only has master
    };
    HAL_CAN_ConfigFilter(&hcan, &filter);
    __HAL_CAN_ENABLE_IT(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_Start(&hcan);
}

static CAN_TxHeaderTypeDef txHeader = {
    0x051,         // StdId
    0,             // ExtId
    CAN_ID_STD,    // IDE
    CAN_RTR_DATA,  // RTR
    8,             // DLC
    DISABLE,       // TransmitGlobalTime
};

void feedbackPowerData()
{
    txData.errorCode         = PowerManager::Status::status.errorCode | ((uint8_t)!PowerManager::Status::status.outputEnabled) << 7;
    txData.chassisPower      = PowerManager::Status::status.chassisPower;
    txData.chassisPowerLimit = PowerManager::Status::status.chassisPowerLimit;
    txData.capEnergy         = PowerManager::Status::status.capEnergy > 1.0f ? 255 : PowerManager::Status::status.capEnergy * 255;
    // HAL_CAN_AddTxMessage(&hcan, &txHeader, (uint8_t *)&txData, &txMailbox);
    if ((READ_REG(hcan.Instance->TSR) & CAN_TSR_TME0) != 0U)
        SET_BIT(hcan.Instance->TSR, CAN_TSR_ABRQ0);
    hcan.Instance->sTxMailBox[CAN_TX_MAILBOX0].TIR  = ((txHeader.StdId << CAN_TI0R_STID_Pos) | txHeader.RTR);
    hcan.Instance->sTxMailBox[CAN_TX_MAILBOX0].TDTR = (txHeader.DLC);
    WRITE_REG(hcan.Instance->sTxMailBox[CAN_TX_MAILBOX0].TDHR,
              ((uint32_t)(((uint8_t *)&txData)[7]) << CAN_TDH0R_DATA7_Pos) | ((uint32_t)(((uint8_t *)&txData)[6]) << CAN_TDH0R_DATA6_Pos) |
                  ((uint32_t)(((uint8_t *)&txData)[5]) << CAN_TDH0R_DATA5_Pos) | ((uint32_t)(((uint8_t *)&txData)[4]) << CAN_TDH0R_DATA4_Pos));
    WRITE_REG(hcan.Instance->sTxMailBox[CAN_TX_MAILBOX0].TDLR,
              ((uint32_t)(((uint8_t *)&txData)[3]) << CAN_TDL0R_DATA3_Pos) | ((uint32_t)(((uint8_t *)&txData)[2]) << CAN_TDL0R_DATA2_Pos) |
                  ((uint32_t)(((uint8_t *)&txData)[1]) << CAN_TDL0R_DATA1_Pos) | ((uint32_t)(((uint8_t *)&txData)[0]) << CAN_TDL0R_DATA0_Pos));
    SET_BIT(hcan.Instance->sTxMailBox[CAN_TX_MAILBOX0].TIR, CAN_TI0R_TXRQ);
}

CAN_RxHeaderTypeDef rxHeader;

extern "C"
{
    void CAN_RX0_IRQHandler(void)
    {
        if ((hcan.Instance->RF0R & CAN_RF0R_FMP0) != 0U)
        {
            while (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, (uint8_t *)&rxData) == HAL_OK)
                if ((rxHeader.StdId == 0x061) && (rxHeader.DLC == 8) && (rxHeader.IDE == CAN_ID_STD))
                {
                    static bool lastEnableDCDC = true;

                    if (rxData.systemRestart)
                        PowerManager::systemRestart();

                    PowerManager::ControlData::controlData.refereePowerLimit = M_CLAMP(rxData.feedbackRefereePowerLimit, 30.0f, 250.0f);
                    PowerManager::ControlData::controlData.energyRemain      = M_CLAMP(rxData.feedbackRefereeEnergyBuffer, 0.0f, 300.0f);

                    if (rxData.enableDCDC == false || rxData.feedbackRefereePowerLimit > 250.0f)
                        PowerManager::ControlData::controlData.enableOutput = false;
                    else if (lastEnableDCDC == false)
                        PowerManager::ControlData::controlData.enableOutput = true;
                    lastEnableDCDC = rxData.enableDCDC;

                    PowerManager::updateEnergy();
                }
        }
    }
}

}  // namespace Communication