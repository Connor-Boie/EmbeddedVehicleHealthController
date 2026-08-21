#include "CanBus.hpp"

CanBus::CanBus(
    CAN_HandleTypeDef* can)
    : can_{can}
{
}

bool CanBus::initialize()
{
    initialized_ = false;

    if (can_ == nullptr)
    {
        ++receiveFailureCount_;
        return false;
    }

    CAN_FilterTypeDef filter{};

    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;

    if (HAL_CAN_ConfigFilter(
            can_,
            &filter) != HAL_OK)
    {
        ++receiveFailureCount_;
        return false;
    }

    if (HAL_CAN_Start(can_) != HAL_OK)
    {
        ++receiveFailureCount_;
        return false;
    }

    initialized_ = true;
    return true;
}

bool CanBus::send(
    const CanFrame& frame)
{
    if ((!initialized_) ||
        (can_ == nullptr) ||
        (frame.id > MaximumStandardId) ||
        (frame.length > MaximumPayloadLength))
    {
        ++transmitFailureCount_;
        return false;
    }

    CAN_TxHeaderTypeDef header{};

    header.StdId = frame.id;
    header.ExtId = 0U;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = frame.length;
    header.TransmitGlobalTime = DISABLE;

    std::uint8_t payload[MaximumPayloadLength]{};

    for (std::uint8_t index = 0U;
         index < frame.length;
         ++index)
    {
        payload[index] = frame.data[index];
    }

    std::uint32_t mailbox = 0U;

    if (HAL_CAN_AddTxMessage(
            can_,
            &header,
            payload,
            &mailbox) != HAL_OK)
    {
        ++transmitFailureCount_;
        return false;
    }

    ++transmitCount_;
    return true;
}

bool CanBus::receive(
    CanFrame& frame)
{
    if ((!initialized_) ||
        (can_ == nullptr))
    {
        ++receiveFailureCount_;
        return false;
    }

    if (!messagePending())
    {
        return false;
    }

    CAN_RxHeaderTypeDef header{};
    std::uint8_t payload[MaximumPayloadLength]{};

    if (HAL_CAN_GetRxMessage(
            can_,
            CAN_RX_FIFO0,
            &header,
            payload) != HAL_OK)
    {
        ++receiveFailureCount_;
        return false;
    }

    if ((header.IDE != CAN_ID_STD) ||
        (header.RTR != CAN_RTR_DATA) ||
        (header.DLC > MaximumPayloadLength))
    {
        ++receiveFailureCount_;
        return false;
    }

    frame.id = header.StdId;
    frame.length =
        static_cast<std::uint8_t>(
            header.DLC);

    for (std::uint8_t index = 0U;
         index < MaximumPayloadLength;
         ++index)
    {
        frame.data[index] =
            (index < frame.length)
                ? payload[index]
                : 0U;
    }

    ++receiveCount_;
    return true;
}

bool CanBus::initialized() const
{
    return initialized_;
}

bool CanBus::messagePending() const
{
    if ((!initialized_) ||
        (can_ == nullptr))
    {
        return false;
    }

    return HAL_CAN_GetRxFifoFillLevel(
               can_,
               CAN_RX_FIFO0) > 0U;
}

std::uint32_t
CanBus::transmitCount() const
{
    return transmitCount_;
}

std::uint32_t
CanBus::transmitFailureCount() const
{
    return transmitFailureCount_;
}

std::uint32_t
CanBus::receiveCount() const
{
    return receiveCount_;
}

std::uint32_t
CanBus::receiveFailureCount() const
{
    return receiveFailureCount_;
}
