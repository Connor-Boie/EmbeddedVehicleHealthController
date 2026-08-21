#include "DiagnosticLogger.hpp"

#include <cstddef>

DiagnosticLogger::DiagnosticLogger(
    W25q64* flash)
    : flash_{flash}
{
}

bool DiagnosticLogger::initialize()
{
    initialized_ = false;
    full_ = false;

    nextWriteAddress_ =
        RegionStartAddress;

    nextSequence_ = 0U;
    recordCount_ = 0U;
    invalidRecordCount_ = 0U;
    lastRecord_ = {};
    lastRecordValid_ = false;

    if ((flash_ == nullptr) ||
        !flash_->available())
    {
        ++failureCount_;
        return false;
    }

    std::uint32_t firstErasedAddress =
        RegionEndAddress;

    for (std::uint32_t address =
             RegionStartAddress;
         address < RegionEndAddress;
         address += RecordSizeBytes)
    {
        DiagnosticRecord record{};

        if (!readRecord(address, record))
        {
            ++failureCount_;
            return false;
        }

        if (isErased(record))
        {
            if (firstErasedAddress ==
                RegionEndAddress)
            {
                firstErasedAddress =
                    address;
            }

            continue;
        }

        if (!isValid(record))
        {
            ++invalidRecordCount_;
            continue;
        }

        ++recordCount_;

        if ((!lastRecordValid_) ||
            (record.sequence >=
             lastRecord_.sequence))
        {
            lastRecord_ = record;
            lastRecordValid_ = true;
        }

        if (record.sequence >=
            nextSequence_)
        {
            nextSequence_ =
                record.sequence + 1U;
        }
    }

    if (firstErasedAddress ==
        RegionEndAddress)
    {
        full_ = true;
        nextWriteAddress_ =
            RegionEndAddress;
    }
    else
    {
        nextWriteAddress_ =
            firstErasedAddress;
    }

    initialized_ = true;
    return true;
}

bool DiagnosticLogger::append(
    DiagnosticEventType eventType,
    std::uint32_t uptimeMs,
    std::uint32_t data0,
    std::uint32_t data1)
{
    if ((!initialized_) ||
        (flash_ == nullptr) ||
        !flash_->available())
    {
        ++failureCount_;
        return false;
    }

    if (full_)
    {
        return false;
    }

    DiagnosticRecord record{};

    record.magic = RecordMagic;
    record.formatVersion =
        RecordFormatVersion;
    record.eventType =
        static_cast<std::uint32_t>(
            eventType);
    record.sequence = nextSequence_;
    record.uptimeMs = uptimeMs;
    record.data0 = data0;
    record.data1 = data1;
    record.checksum =
        calculateChecksum(record);

    if (!flash_->program(
        nextWriteAddress_,
        reinterpret_cast<
            const std::uint8_t*>(&record),
        sizeof(record)))
    {
        ++failureCount_;
        return false;
    }

    DiagnosticRecord readback{};

    if (!readRecord(
        nextWriteAddress_,
        readback))
    {
        ++failureCount_;
        return false;
    }

    if (!isValid(readback) ||
        (readback.sequence !=
         record.sequence))
    {
        ++failureCount_;
        return false;
    }

    ++recordCount_;
    ++nextSequence_;

    lastRecord_ = readback;
    lastRecordValid_ = true;

    const std::uint32_t searchAddress =
        nextWriteAddress_ +
        RecordSizeBytes;

    std::uint32_t erasedAddress =
        RegionEndAddress;

    if (!findNextErasedAddress(
        searchAddress,
        erasedAddress))
    {
        ++failureCount_;
        return false;
    }

    if (erasedAddress ==
        RegionEndAddress)
    {
        full_ = true;
        nextWriteAddress_ =
            RegionEndAddress;
    }
    else
    {
        nextWriteAddress_ =
            erasedAddress;
    }

    return true;
}

bool DiagnosticLogger::eraseAll()
{
    initialized_ = false;

    if ((flash_ == nullptr) ||
        !flash_->available())
    {
        ++failureCount_;
        return false;
    }

    for (std::uint32_t sector = 0U;
         sector < RegionSectorCount;
         ++sector)
    {
        const std::uint32_t address =
            RegionStartAddress +
            (sector *
             W25q64::SectorSizeBytes);

        if (!flash_->eraseSector(address))
        {
            ++failureCount_;
            return false;
        }
    }

    return initialize();
}

bool DiagnosticLogger::initialized() const
{
    return initialized_;
}

bool DiagnosticLogger::full() const
{
    return full_;
}

std::uint32_t
DiagnosticLogger::recordCount() const
{
    return recordCount_;
}

std::uint32_t
DiagnosticLogger::invalidRecordCount() const
{
    return invalidRecordCount_;
}

std::uint32_t
DiagnosticLogger::failureCount() const
{
    return failureCount_;
}

std::uint32_t
DiagnosticLogger::nextSequence() const
{
    return nextSequence_;
}

bool
DiagnosticLogger::lastRecordValid() const
{
    return lastRecordValid_;
}

const DiagnosticRecord&
DiagnosticLogger::lastRecord() const
{
    return lastRecord_;
}

bool DiagnosticLogger::readRecord(
    std::uint32_t address,
    DiagnosticRecord& record)
{
    if (flash_ == nullptr)
    {
        return false;
    }

    return flash_->read(
        address,
        reinterpret_cast<std::uint8_t*>(
            &record),
        sizeof(record));
}

bool DiagnosticLogger::findNextErasedAddress(
    std::uint32_t startAddress,
    std::uint32_t& erasedAddress)
{
    erasedAddress = RegionEndAddress;

    if (startAddress >=
        RegionEndAddress)
    {
        return true;
    }

    for (std::uint32_t address =
             startAddress;
         address < RegionEndAddress;
         address += RecordSizeBytes)
    {
        DiagnosticRecord record{};

        if (!readRecord(address, record))
        {
            return false;
        }

        if (isErased(record))
        {
            erasedAddress = address;
            return true;
        }
    }

    return true;
}

bool DiagnosticLogger::isErased(
    const DiagnosticRecord& record)
{
    return
        (record.magic == ErasedWord) &&
        (record.formatVersion == ErasedWord) &&
        (record.eventType == ErasedWord) &&
        (record.sequence == ErasedWord) &&
        (record.uptimeMs == ErasedWord) &&
        (record.data0 == ErasedWord) &&
        (record.data1 == ErasedWord) &&
        (record.checksum == ErasedWord);
}

bool DiagnosticLogger::isValid(
    const DiagnosticRecord& record)
{
    if ((record.magic != RecordMagic) ||
        (record.formatVersion !=
         RecordFormatVersion))
    {
        return false;
    }

    return record.checksum ==
        calculateChecksum(record);
}

std::uint32_t
DiagnosticLogger::calculateChecksum(
    const DiagnosticRecord& record)
{
    std::uint32_t checksum =
        FnvOffsetBasis;

    checksum = updateChecksum(
        checksum,
        record.magic);

    checksum = updateChecksum(
        checksum,
        record.formatVersion);

    checksum = updateChecksum(
        checksum,
        record.eventType);

    checksum = updateChecksum(
        checksum,
        record.sequence);

    checksum = updateChecksum(
        checksum,
        record.uptimeMs);

    checksum = updateChecksum(
        checksum,
        record.data0);

    checksum = updateChecksum(
        checksum,
        record.data1);

    return checksum;
}

std::uint32_t
DiagnosticLogger::updateChecksum(
    std::uint32_t checksum,
    std::uint32_t value)
{
    for (std::uint32_t byteIndex = 0U;
         byteIndex < 4U;
         ++byteIndex)
    {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(
                (value >>
                 (byteIndex * 8U)) &
                0xFFU);

        checksum ^= byte;
        checksum *= FnvPrime;
    }

    return checksum;
}
