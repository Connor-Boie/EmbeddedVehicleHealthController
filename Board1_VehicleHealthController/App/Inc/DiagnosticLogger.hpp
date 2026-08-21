#ifndef DIAGNOSTIC_LOGGER_HPP
#define DIAGNOSTIC_LOGGER_HPP

#include "W25q64.hpp"

#include <cstdint>

enum class DiagnosticEventType : std::uint32_t
{
    SystemStartup = 1U,
    FaultActivated = 2U,
    FaultCleared = 3U
};

struct DiagnosticRecord
{
    std::uint32_t magic;
    std::uint32_t formatVersion;
    std::uint32_t eventType;
    std::uint32_t sequence;
    std::uint32_t uptimeMs;
    std::uint32_t data0;
    std::uint32_t data1;
    std::uint32_t checksum;
};

static_assert(
    sizeof(DiagnosticRecord) == 32U,
    "DiagnosticRecord must remain 32 bytes");

class DiagnosticLogger
{
public:
    explicit DiagnosticLogger(
        W25q64* flash);

    [[nodiscard]] bool initialize();

    [[nodiscard]] bool append(
        DiagnosticEventType eventType,
        std::uint32_t uptimeMs,
        std::uint32_t data0,
        std::uint32_t data1);

    [[nodiscard]] bool eraseAll();

    [[nodiscard]] bool initialized() const;
    [[nodiscard]] bool full() const;

    [[nodiscard]] std::uint32_t
        recordCount() const;

    [[nodiscard]] std::uint32_t
        invalidRecordCount() const;

    [[nodiscard]] std::uint32_t
        failureCount() const;

    [[nodiscard]] std::uint32_t
        nextSequence() const;

    [[nodiscard]] bool
        lastRecordValid() const;

    [[nodiscard]] const DiagnosticRecord&
        lastRecord() const;

    static constexpr std::uint32_t
        RegionStartAddress = 0x000000U;

    static constexpr std::uint32_t
        RegionSectorCount = 2U;

    static constexpr std::uint32_t
        RegionSizeBytes =
            RegionSectorCount *
            W25q64::SectorSizeBytes;

    static constexpr std::uint32_t
        RegionEndAddress =
            RegionStartAddress +
            RegionSizeBytes;

    static constexpr std::uint32_t
        RecordSizeBytes =
            sizeof(DiagnosticRecord);

    static constexpr std::uint32_t
        RecordCapacity =
            RegionSizeBytes /
            RecordSizeBytes;

private:
    [[nodiscard]] bool readRecord(
        std::uint32_t address,
        DiagnosticRecord& record);

    [[nodiscard]] bool findNextErasedAddress(
        std::uint32_t startAddress,
        std::uint32_t& erasedAddress);

    [[nodiscard]] static bool isErased(
        const DiagnosticRecord& record);

    [[nodiscard]] static bool isValid(
        const DiagnosticRecord& record);

    [[nodiscard]] static std::uint32_t
        calculateChecksum(
            const DiagnosticRecord& record);

    [[nodiscard]] static std::uint32_t
        updateChecksum(
            std::uint32_t checksum,
            std::uint32_t value);

    static constexpr std::uint32_t
        RecordMagic = 0x474C5645U;

    static constexpr std::uint32_t
        RecordFormatVersion = 1U;

    static constexpr std::uint32_t
        ErasedWord = 0xFFFFFFFFU;

    static constexpr std::uint32_t
        FnvOffsetBasis = 2166136261U;

    static constexpr std::uint32_t
        FnvPrime = 16777619U;

    W25q64* flash_;

    std::uint32_t nextWriteAddress_{
        RegionStartAddress};

    std::uint32_t nextSequence_{0U};
    std::uint32_t recordCount_{0U};
    std::uint32_t invalidRecordCount_{0U};
    std::uint32_t failureCount_{0U};

    DiagnosticRecord lastRecord_{};

    bool initialized_{false};
    bool full_{false};
    bool lastRecordValid_{false};
};

#endif
