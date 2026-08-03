#include "ResetCauseDetector.hpp"

#include "stm32f4xx_hal.h"

void ResetCauseDetector::capture()
{
    causeMask_ = 0U;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET)
    {
        causeMask_ |= PowerOnBit;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET)
    {
        causeMask_ |= BrownoutBit;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
    {
        causeMask_ |= ExternalPinBit;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET)
    {
        causeMask_ |= SoftwareBit;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
    {
        causeMask_ |= IndependentWatchdogBit;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET)
    {
        causeMask_ |= WindowWatchdogBit;
    }

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET)
    {
        causeMask_ |= LowPowerBit;
    }

    primaryCause_ = selectPrimaryCause(causeMask_);

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

ResetCause ResetCauseDetector::primaryCause() const
{
    return primaryCause_;
}

std::uint32_t ResetCauseDetector::causeMask() const
{
    return causeMask_;
}

const char* ResetCauseDetector::primaryCauseName() const
{
    return causeName(primaryCause_);
}

ResetCause ResetCauseDetector::selectPrimaryCause(
    std::uint32_t causeMask)
{
	if ((causeMask & IndependentWatchdogBit) != 0U)
	{
	    return ResetCause::IndependentWatchdog;
	}

	if ((causeMask & WindowWatchdogBit) != 0U)
	{
	    return ResetCause::WindowWatchdog;
	}

	if ((causeMask & SoftwareBit) != 0U)
	{
	    return ResetCause::Software;
	}

	if ((causeMask & LowPowerBit) != 0U)
	{
	    return ResetCause::LowPower;
	}

	if ((causeMask & PowerOnBit) != 0U)
	{
	    return ResetCause::PowerOn;
	}

	if ((causeMask & BrownoutBit) != 0U)
	{
	    return ResetCause::Brownout;
	}

	if ((causeMask & ExternalPinBit) != 0U)
	{
	    return ResetCause::ExternalPin;
	}

    return ResetCause::Unknown;
}

const char* ResetCauseDetector::causeName(
    ResetCause cause)
{
    switch (cause)
    {
        case ResetCause::PowerOn:
            return "POWER_ON";

        case ResetCause::Brownout:
            return "BROWNOUT";

        case ResetCause::ExternalPin:
            return "EXTERNAL_PIN";

        case ResetCause::Software:
            return "SOFTWARE";

        case ResetCause::IndependentWatchdog:
            return "INDEPENDENT_WATCHDOG";

        case ResetCause::WindowWatchdog:
            return "WINDOW_WATCHDOG";

        case ResetCause::LowPower:
            return "LOW_POWER";

        case ResetCause::Unknown:
        default:
            return "UNKNOWN";
    }
}
