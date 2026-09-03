#include "RgbLedPwm.hpp"

#include <cstdint>

RgbLedPwm::RgbLedPwm(
    TIM_HandleTypeDef* timer,
    std::uint32_t redChannel,
    std::uint32_t greenChannel,
    std::uint32_t blueChannel)
    : timer_{timer},
      redChannel_{redChannel},
      greenChannel_{greenChannel},
      blueChannel_{blueChannel}
{
}

bool RgbLedPwm::initialize()
{
    initialized_ = false;

    intensity_ = {};

    if (timer_ == nullptr)
    {
        ++failureCount_;
        return false;
    }

    __HAL_TIM_SET_COMPARE(
        timer_,
        redChannel_,
        0U);

    __HAL_TIM_SET_COMPARE(
        timer_,
        greenChannel_,
        0U);

    __HAL_TIM_SET_COMPARE(
        timer_,
        blueChannel_,
        0U);

    const HAL_StatusTypeDef
        redStatus =
            HAL_TIM_PWM_Start(
                timer_,
                redChannel_);

    const HAL_StatusTypeDef
        greenStatus =
            HAL_TIM_PWM_Start(
                timer_,
                greenChannel_);

    const HAL_StatusTypeDef
        blueStatus =
            HAL_TIM_PWM_Start(
                timer_,
                blueChannel_);

    if ((redStatus != HAL_OK) ||
        (greenStatus != HAL_OK) ||
        (blueStatus != HAL_OK))
    {
        static_cast<void>(
            HAL_TIM_PWM_Stop(
                timer_,
                redChannel_));

        static_cast<void>(
            HAL_TIM_PWM_Stop(
                timer_,
                greenChannel_));

        static_cast<void>(
            HAL_TIM_PWM_Stop(
                timer_,
                blueChannel_));

        ++failureCount_;

        return false;
    }

    initialized_ = true;

    setIntensity({});

    return true;
}

void RgbLedPwm::setColor(
    WarningColor color)
{
    setIntensity(
        intensityForColor(color));
}

void RgbLedPwm::setIntensity(
    const RgbIntensityPercent&
        intensity)
{
    intensity_.red =
        (intensity.red <= 100U)
            ? intensity.red
            : 100U;

    intensity_.green =
        (intensity.green <= 100U)
            ? intensity.green
            : 100U;

    intensity_.blue =
        (intensity.blue <= 100U)
            ? intensity.blue
            : 100U;

    if (!initialized_)
    {
        return;
    }

    applyIntensity();
}

RgbIntensityPercent
RgbLedPwm::intensityForColor(
    WarningColor color)
{
    switch (color)
    {
        case WarningColor::Green:
        {
            return {0U, 100U, 0U};
        }

        case WarningColor::Yellow:
        {
            return {100U, 25U, 0U};
        }

        case WarningColor::Blue:
        {
            return {0U, 0U, 100U};
        }

        case WarningColor::Orange:
        {
            return {100U, 5U, 0U};
        }

        case WarningColor::Red:
        {
            return {100U, 0U, 0U};
        }

        case WarningColor::Magenta:
        {
            return {100U, 0U, 80U};
        }
    }

    return {};
}

const RgbIntensityPercent&
RgbLedPwm::intensity() const
{
    return intensity_;
}

bool RgbLedPwm::initialized() const
{
    return initialized_;
}

std::uint32_t
RgbLedPwm::failureCount() const
{
    return failureCount_;
}

std::uint32_t
RgbLedPwm::compareForPercent(
    std::uint8_t percent) const
{
    if (timer_ == nullptr)
    {
        return 0U;
    }

    const std::uint32_t
        periodCounts =
            __HAL_TIM_GET_AUTORELOAD(
                timer_) +
            1U;

    const std::uint8_t
        boundedPercent =
            (percent <= 100U)
                ? percent
                : 100U;

    const std::uint64_t
        scaledCompare =
            static_cast<std::uint64_t>(
                periodCounts) *
            boundedPercent;

    return static_cast<std::uint32_t>(
        scaledCompare /
        100U);
}

void RgbLedPwm::applyIntensity()
{
    __HAL_TIM_SET_COMPARE(
        timer_,
        redChannel_,
        compareForPercent(
            intensity_.red));

    __HAL_TIM_SET_COMPARE(
        timer_,
        greenChannel_,
        compareForPercent(
            intensity_.green));

    __HAL_TIM_SET_COMPARE(
        timer_,
        blueChannel_,
        compareForPercent(
            intensity_.blue));
}
