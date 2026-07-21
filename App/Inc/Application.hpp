#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <cstdint>

class Application
{
public:
    void initialize();
    void run();

    [[nodiscard]] std::uint32_t blinkCount() const;

private:
    std::uint32_t blinkCount_{0U};
};

#endif
