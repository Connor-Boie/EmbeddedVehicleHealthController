#ifndef RGB_LED_PWM_HPP
#define RGB_LED_PWM_HPP

#include "ActuatorCommandPolicy.hpp"
#include "stm32f4xx_hal.h"

#include <cstdint>

struct RgbIntensityPercent
{
    std::uint8_t red{0U};
    std::uint8_t green{0U};
    std::uint8_t blue{0U};
};

class RgbLedPwm
{
public:
    RgbLedPwm(
        TIM_HandleTypeDef* timer,
        std::uint32_t redChannel,
        std::uint32_t greenChannel,
        std::uint32_t blueChannel);

    [[nodiscard]] bool initialize();

    void setColor(
        WarningColor color);

    void setIntensity(
        const RgbIntensityPercent&
            intensity);

    [[nodiscard]] static
        RgbIntensityPercent
        intensityForColor(
            WarningColor color);

    [[nodiscard]] const
        RgbIntensityPercent&
        intensity() const;

    [[nodiscard]] bool
        initialized() const;

    [[nodiscard]] std::uint32_t
        failureCount() const;

private:
    [[nodiscard]] std::uint32_t
        compareForPercent(
            std::uint8_t percent) const;

    void applyIntensity();

    TIM_HandleTypeDef* timer_;

    std::uint32_t redChannel_;
    std::uint32_t greenChannel_;
    std::uint32_t blueChannel_;

    RgbIntensityPercent intensity_{};

    bool initialized_{false};

    std::uint32_t failureCount_{0U};
};

#endif
