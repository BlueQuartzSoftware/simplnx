#pragma once

#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"

#include <atomic>
#include <filesystem>
#include <string_view>

#if SIMPLNX_BUILD_TESTS
namespace nx::core::HDF5
{
/**
 * @struct CodecOwnershipCountersForTesting
 * @brief Collects concurrent payload lifetimes for one isolated observation scope.
 */
struct CodecOwnershipCountersForTesting
{
  std::atomic<uint64> live{0};
  std::atomic<uint64> peak{0};
  std::atomic<uint64> nominalAcquired{0};
  std::atomic<uint64> edgeAcquired{0};
  std::atomic<uint64> transfers{0};
  std::atomic<bool> invalidRelease{false};

  /**
   * @brief Records one event without allocation, locks or storage reentry.
   * @param context Points to the live fixed counter record.
   * @param file Borrows the event identity; this isolated scope observes every event.
   * @param dataset Borrows the dataset identity.
   * @param event Identifies acquisition, destruction or transfer.
   * @param bytes Supplies the actual observed capacity.
   * @param category Identifies the legacy payload kind when applicable.
   */
  static void Observe(void* context, const std::filesystem::path& file, std::string_view dataset, CodecIoEventForTesting event, uint64 bytes, uint64 category) noexcept
  {
    (void)file;
    (void)dataset;
    auto& self = *static_cast<CodecOwnershipCountersForTesting*>(context);
    if(event == CodecIoEventForTesting::LegacyAcquired)
    {
      if(category == static_cast<uint64>(LegacyPayloadKindForTesting::Nominal))
      {
        self.nominalAcquired.fetch_add(1, std::memory_order_relaxed);
      }
      else if(category == static_cast<uint64>(LegacyPayloadKindForTesting::Edge))
      {
        self.edgeAcquired.fetch_add(1, std::memory_order_relaxed);
      }
    }
    if(event == CodecIoEventForTesting::LegacyTransferred)
    {
      self.transfers.fetch_add(1, std::memory_order_relaxed);
    }
    if(event == CodecIoEventForTesting::RawAcquired || event == CodecIoEventForTesting::LegacyAcquired)
    {
      const uint64 current = self.live.fetch_add(bytes, std::memory_order_relaxed) + bytes;
      uint64 peakValue = self.peak.load(std::memory_order_relaxed);
      while(current > peakValue && !self.peak.compare_exchange_weak(peakValue, current, std::memory_order_relaxed))
      {
      }
    }
    else if(event == CodecIoEventForTesting::RawReleased || event == CodecIoEventForTesting::LegacyReleased || event == CodecIoEventForTesting::LegacyTransferred)
    {
      uint64 current = self.live.load(std::memory_order_relaxed);
      while(true)
      {
        if(bytes > current)
        {
          self.invalidRelease.store(true, std::memory_order_relaxed);
          break;
        }
        if(self.live.compare_exchange_weak(current, current - bytes, std::memory_order_relaxed))
        {
          break;
        }
      }
    }
  }
};
} // namespace nx::core::HDF5
#endif
