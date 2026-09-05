#include "FanPwm.hpp"

#include <cstdint>

FanPwm::FanPwm(
    TIM_HandleTypeDef* timer,
    std::uint32_t channel)
    : timer_{timer},
      channel_{channel}
{
}

bool FanPwm::initialize()
{
    initialized_ = false;

    dutyPercent_ = 0U;

    if (timer_ == nullptr)
    {
        ++failureCount_;
        return false;
    }

    __HAL_TIM_SET_COMPARE(
        timer_,
        channel_,
        0U);

    const HAL_StatusTypeDef status =
        HAL_TIM_PWM_Start(
            timer_,
            channel_);

    if (status != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    initialized_ = true;

    setDutyPercent(0U);

    return true;
}

void FanPwm::setDutyPercent(
    std::uint8_t dutyPercent)
{
    dutyPercent_ =
        (dutyPercent <= 100U)
            ? dutyPercent
            : 100U;

    if (!initialized_)
    {
        return;
    }

    applyDuty();
}

std::uint8_t FanPwm::dutyPercent() const
{
    return dutyPercent_;
}

bool FanPwm::initialized() const
{
    return initialized_;
}

std::uint32_t FanPwm::failureCount() const
{
    return failureCount_;
}

std::uint32_t FanPwm::compareForPercent(
    std::uint8_t percent) const
{
    if (timer_ == nullptr)
    {
        return 0U;
    }

    const std::uint32_t periodCounts =
        __HAL_TIM_GET_AUTORELOAD(
            timer_) +
        1U;

    const std::uint8_t boundedPercent =
        (percent <= 100U)
            ? percent
            : 100U;

    const std::uint64_t scaledCompare =
        static_cast<std::uint64_t>(
            periodCounts) *
        boundedPercent;

    return static_cast<std::uint32_t>(
        scaledCompare /
        100U);
}

void FanPwm::applyDuty()
{
    __HAL_TIM_SET_COMPARE(
        timer_,
        channel_,
        compareForPercent(
            dutyPercent_));
}
