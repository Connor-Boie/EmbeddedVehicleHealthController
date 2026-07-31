#include "Watchdog.hpp"

Watchdog::Watchdog(IWDG_HandleTypeDef* watchdog)
    : watchdog_{watchdog}
{
}

bool Watchdog::refresh()
{
    if (watchdog_ == nullptr)
    {
        ++failureCount_;
        return false;
    }

    const HAL_StatusTypeDef refreshStatus =
        HAL_IWDG_Refresh(watchdog_);

    if (refreshStatus != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    ++refreshCount_;
    return true;
}

std::uint32_t Watchdog::refreshCount() const
{
    return refreshCount_;
}

std::uint32_t Watchdog::failureCount() const
{
    return failureCount_;
}
