#include "UartCommandReceiver.hpp"

void UartCommandReceiver::onByteReceivedFromInterrupt(std::uint8_t byte)
{
    const std::size_t nextHead =
        (receiveHead_ + 1U) % ReceiveBufferCapacity;

    if (nextHead == receiveTail_)
    {
        ++droppedByteCount_;
        return;
    }

    receiveBuffer_[receiveHead_] = byte;
    receiveHead_ = nextHead;
}

void UartCommandReceiver::onReceiveErrorFromInterrupt()
{
    ++receiveErrorCount_;
}

void UartCommandReceiver::process()
{
    std::uint8_t byte = 0U;

    while (popReceivedByte(byte))
    {
        processByte(byte);
    }
}

bool UartCommandReceiver::readLine(
    char* destination,
    std::size_t destinationCapacity)
{
    if ((!pendingLineAvailable_) ||
        (destination == nullptr) ||
        (destinationCapacity == 0U))
    {
        return false;
    }

    if (destinationCapacity <= pendingLineLength_)
    {
        return false;
    }

    for (std::size_t index = 0U;
         index < pendingLineLength_;
         ++index)
    {
        destination[index] = pendingLine_[index];
    }

    destination[pendingLineLength_] = '\0';

    pendingLineAvailable_ = false;
    pendingLineLength_ = 0U;

    return true;
}

std::uint32_t UartCommandReceiver::droppedByteCount() const
{
    return droppedByteCount_;
}

std::uint32_t UartCommandReceiver::overflowLineCount() const
{
    return overflowLineCount_;
}

std::uint32_t UartCommandReceiver::droppedLineCount() const
{
    return droppedLineCount_;
}

std::uint32_t UartCommandReceiver::receiveErrorCount() const
{
    return receiveErrorCount_;
}

bool UartCommandReceiver::popReceivedByte(std::uint8_t& byte)
{
    if (receiveTail_ == receiveHead_)
    {
        return false;
    }

    byte = receiveBuffer_[receiveTail_];

    receiveTail_ =
        (receiveTail_ + 1U) % ReceiveBufferCapacity;

    return true;
}

void UartCommandReceiver::processByte(std::uint8_t byte)
{
	if ((byte == static_cast<std::uint8_t>('\r')) ||
	    (byte == static_cast<std::uint8_t>('\n')))
	{
	    finishCurrentLine();
	    return;
	}

    if (workingLineOverflow_)
    {
        return;
    }

    if (workingLineLength_ >= (LineCapacity - 1U))
    {
        workingLineOverflow_ = true;
        return;
    }

    workingLine_[workingLineLength_] =
        static_cast<char>(byte);

    ++workingLineLength_;
}

void UartCommandReceiver::finishCurrentLine()
{
    if (workingLineOverflow_)
    {
        ++overflowLineCount_;

        workingLineLength_ = 0U;
        workingLineOverflow_ = false;

        return;
    }

    if (workingLineLength_ == 0U)
    {
        return;
    }

    if (pendingLineAvailable_)
    {
        ++droppedLineCount_;
    }
    else
    {
        for (std::size_t index = 0U;
             index < workingLineLength_;
             ++index)
        {
            pendingLine_[index] = workingLine_[index];
        }

        pendingLine_[workingLineLength_] = '\0';
        pendingLineLength_ = workingLineLength_;
        pendingLineAvailable_ = true;
    }

    workingLineLength_ = 0U;
}
