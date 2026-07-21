#ifndef BUTTON_DEBOUNCER_HPP
#define BUTTON_DEBOUNCER_HPP

#include <cstdint>

class ButtonDebouncer
{
public:
	explicit ButtonDebouncer(std::uint32_t debouncePeriodMs);

	void initialize(bool initialPressed, std::uint32_t currentTimeMs);
	void update(bool rawPressed, std::uint32_t currentTimeMs);

	[[nodiscard]] bool isPressed() const;
	[[nodiscard]] bool pressedEvent() const;
	[[nodiscard]] bool releasedEvent() const;

private:
	std::uint32_t debouncePeriodMs_;

	bool rawPressed_{false};
	bool confirmedPressed_{false};

	bool pressedEvent_{false};
	bool releasedEvent_{false};

	std::uint32_t lastRawChangeTimeMs_{0U};
};

#endif
