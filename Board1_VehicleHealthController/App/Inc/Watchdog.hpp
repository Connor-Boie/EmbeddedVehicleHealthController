#ifndef WATCHDOG_HPP
#define WATCHDOG_HPP

#include "stm32f4xx_hal.h"

#include <cstdint>

class Watchdog
{
public:
    explicit Watchdog(IWDG_HandleTypeDef* watchdog);

    [[nodiscard]] bool refresh();

    [[nodiscard]] std::uint32_t refreshCount() const;
    [[nodiscard]] std::uint32_t failureCount() const;

private:
    IWDG_HandleTypeDef* watchdog_;

    std::uint32_t refreshCount_{0U};
    std::uint32_t failureCount_{0U};
};

#endif
