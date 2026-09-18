#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"

#include <algorithm>
#include <atomic>

namespace nx::core::ImageProcessing
{
namespace
{
std::atomic<uint64> s_WorkingMemoryTuningOverrideBytes = 0;
}

ScopedWorkingMemoryTuningOverride::ScopedWorkingMemoryTuningOverride(uint64 sizeBytes) noexcept
: m_PreviousSizeBytes(s_WorkingMemoryTuningOverrideBytes.exchange(sizeBytes, std::memory_order_acq_rel))
{
}

ScopedWorkingMemoryTuningOverride::~ScopedWorkingMemoryTuningOverride() noexcept
{
  s_WorkingMemoryTuningOverrideBytes.store(m_PreviousSizeBytes, std::memory_order_release);
}

uint64 ResolvePreferredWorkingMemoryBytes(uint64 preferredBytes) noexcept
{
  const uint64 overrideBytes = s_WorkingMemoryTuningOverrideBytes.load(std::memory_order_acquire);
  return overrideBytes == 0 ? preferredBytes : overrideBytes;
}

uint64 ResolveWorkingMemoryFractionBytes(uint64 usefulBytes, uint32 numerator, uint32 denominator, uint64 minimumBytes) noexcept
{
  if(usefulBytes == 0 || denominator == 0)
  {
    return 0;
  }

  const uint64 boundedNumerator = std::min<uint64>(numerator, denominator);
  const uint64 scaledBytes = (usefulBytes / denominator) * boundedNumerator + ((usefulBytes % denominator) * boundedNumerator) / denominator;
  return std::min(usefulBytes, std::max(minimumBytes, scaledBytes));
}

CacheMemoryBudgetManager::WorkingMemoryReservation ReserveWorkingMemory(uint64 preferredBytes, uint64 usefulBytes)
{
  const uint64 requestedBytes = std::min(ResolvePreferredWorkingMemoryBytes(preferredBytes), usefulBytes);
  return CacheMemoryBudgetManager::instance().reserveWorkingMemory(requestedBytes);
}

CacheMemoryBudgetManager::WorkingMemoryReservation ReserveWorkingMemoryFraction(uint64 usefulBytes, uint32 numerator, uint32 denominator, uint64 minimumBytes)
{
  const uint64 preferredBytes = ResolveWorkingMemoryFractionBytes(usefulBytes, numerator, denominator, minimumBytes);
  return ReserveWorkingMemory(preferredBytes, usefulBytes);
}
} // namespace nx::core::ImageProcessing
