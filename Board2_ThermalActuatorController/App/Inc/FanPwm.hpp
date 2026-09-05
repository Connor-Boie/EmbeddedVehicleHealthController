#ifndef FAN_PWM_HPP
#define FAN_PWM_HPP

#include "stm32f4xx_hal.h"

#include <cstdint>

class FanPwm
{
public:
    FanPwm(
        TIM_HandleTypeDef* timer,
        std::uint32_t channel);

    [[nodiscard]] bool initialize();

    void setDutyPercent(
        std::uint8_t dutyPercent);

    [[nodiscard]] std::uint8_t
        dutyPercent() const;

    [[nodiscard]] bool initialized() const;

    [[nodiscard]] std::uint32_t
        failureCount() const;

private:
    [[nodiscard]] std::uint32_t
        compareForPercent(
            std::uint8_t percent) const;

    void applyDuty();

    TIM_HandleTypeDef* timer_;

    std::uint32_t channel_;

    std::uint8_t dutyPercent_{0U};

    bool initialized_{false};

    std::uint32_t failureCount_{0U};
};

#endif
