#include "BuzzerPwm.hpp"

BuzzerPwm::BuzzerPwm(
    TIM_HandleTypeDef* timer,
    std::uint32_t channel)
    : timer_{timer},
      channel_{channel}
{
}

bool BuzzerPwm::initialize()
{
    initialized_ = false;
    enabled_ = false;

    if (timer_ == nullptr)
    {
        return false;
    }

    __HAL_TIM_SET_COMPARE(
        timer_,
        channel_,
        0U);

    if (HAL_TIM_PWM_Start(
            timer_,
            channel_) != HAL_OK)
    {
        return false;
    }

    initialized_ = true;
    return true;
}

void BuzzerPwm::setEnabled(
    bool enabled)
{
    if ((!initialized_) ||
        (timer_ == nullptr))
    {
        enabled_ = false;
        return;
    }

    const std::uint32_t compareValue =
        enabled
            ? enabledCompareValue()
            : 0U;

    __HAL_TIM_SET_COMPARE(
        timer_,
        channel_,
        compareValue);

    enabled_ = enabled;
}

bool BuzzerPwm::initialized() const
{
    return initialized_;
}

bool BuzzerPwm::enabled() const
{
    return enabled_;
}

std::uint32_t
BuzzerPwm::enabledCompareValue() const
{
    if (timer_ == nullptr)
    {
        return 0U;
    }

    const std::uint32_t periodCounts =
        __HAL_TIM_GET_AUTORELOAD(
            timer_) +
        1U;

    return periodCounts / 2U;
}
