#ifndef UART_COMMAND_RECEIVER_HPP
#define UART_COMMAND_RECEIVER_HPP

#include <cstddef>
#include <cstdint>

class UartCommandReceiver
{
public:
    static constexpr std::size_t LineCapacity = 64U;

    void onByteReceivedFromInterrupt(std::uint8_t byte);
    void onReceiveErrorFromInterrupt();

    void process();

    [[nodiscard]] bool readLine(
        char* destination,
        std::size_t destinationCapacity);

    [[nodiscard]] std::uint32_t droppedByteCount() const;
    [[nodiscard]] std::uint32_t overflowLineCount() const;
    [[nodiscard]] std::uint32_t droppedLineCount() const;
    [[nodiscard]] std::uint32_t receiveErrorCount() const;

private:
    static constexpr std::size_t ReceiveBufferCapacity = 128U;

    [[nodiscard]] bool popReceivedByte(std::uint8_t& byte);
    void processByte(std::uint8_t byte);
    void finishCurrentLine();

    std::uint8_t receiveBuffer_[ReceiveBufferCapacity]{};

    volatile std::size_t receiveHead_{0U};
    volatile std::size_t receiveTail_{0U};

    char workingLine_[LineCapacity]{};
    std::size_t workingLineLength_{0U};
    bool workingLineOverflow_{false};

    char pendingLine_[LineCapacity]{};
    std::size_t pendingLineLength_{0U};
    bool pendingLineAvailable_{false};

    volatile std::uint32_t droppedByteCount_{0U};
    volatile std::uint32_t receiveErrorCount_{0U};

    std::uint32_t overflowLineCount_{0U};
    std::uint32_t droppedLineCount_{0U};
};

#endif
