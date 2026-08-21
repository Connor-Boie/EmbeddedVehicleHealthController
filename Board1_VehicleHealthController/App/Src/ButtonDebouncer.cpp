#include "ButtonDebouncer.hpp"

ButtonDebouncer::ButtonDebouncer(std::uint32_t debouncePeriodMs)
	: debouncePeriodMs_{debouncePeriodMs}
{
}

void ButtonDebouncer::initialize(
	bool initialPressed,
	std::uint32_t currentTimeMs)
{
	rawPressed_ = initialPressed;
	confirmedPressed_ = initialPressed;

	pressedEvent_ = false;
	releasedEvent_ = false;

	lastRawChangeTimeMs_ = currentTimeMs;
}

void ButtonDebouncer::update(
	bool rawPressed,
	std::uint32_t currentTimeMs)
{
	pressedEvent_ = false;
	releasedEvent_ = false;

	if (rawPressed != rawPressed_)
	{
		rawPressed_ = rawPressed;
		lastRawChangeTimeMs_ = currentTimeMs;
	}

	const std::uint32_t stableTimeMs =
		currentTimeMs - lastRawChangeTimeMs_;

    if ((rawPressed_ != confirmedPressed_) &&
        (stableTimeMs >= debouncePeriodMs_))
    {
        confirmedPressed_ = rawPressed_;

        if (confirmedPressed_)
        {
            pressedEvent_ = true;
        }
        else
        {
            releasedEvent_ = true;
        }
    }
}

bool ButtonDebouncer::isPressed() const
{
	return confirmedPressed_;
}

bool ButtonDebouncer::pressedEvent() const
{
    return pressedEvent_;
}

bool ButtonDebouncer::releasedEvent() const
{
    return releasedEvent_;
}
