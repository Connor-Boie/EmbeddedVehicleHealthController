#ifndef BUZZER_PWM_HPP
#define BUZZER_PWM_HPP

#include "stm32f4xx_hal.h"

#include <cstdint>

class BuzzerPwm
{
public:
    BuzzerPwm(
        TIM_HandleTypeDef* timer,
        std::uint32_t channel);

    [[nodiscard]] bool initialize();

    void setEnabled(
        bool enabled);

    [[nodiscard]] bool initialized() const;

    [[nodiscard]] bool enabled() const;

private:
    [[nodiscard]] std::uint32_t
        enabledCompareValue() const;

    TIM_HandleTypeDef* timer_;
    std::uint32_t channel_;

    bool initialized_{false};
    bool enabled_{false};
};

#endif
