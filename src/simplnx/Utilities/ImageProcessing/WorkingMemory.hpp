#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

namespace nx::core::ImageProcessing
{
/**
 * @brief Temporarily replaces ImageProcessing's compiled preferred working-memory
 * size for deterministic tests and developer benchmarks.
 *
 * The override changes only the requested preferred size. Useful-size clamping
 * and CacheMemoryBudgetManager's aggregate one-quarter ceiling still apply.
 * Normal application execution creates no override.
 */
class SIMPLNX_EXPORT ScopedWorkingMemoryTuningOverride
{
public:
  explicit ScopedWorkingMemoryTuningOverride(uint64 sizeBytes) noexcept;
  ~ScopedWorkingMemoryTuningOverride() noexcept;

  ScopedWorkingMemoryTuningOverride(const ScopedWorkingMemoryTuningOverride&) = delete;
  ScopedWorkingMemoryTuningOverride& operator=(const ScopedWorkingMemoryTuningOverride&) = delete;
  ScopedWorkingMemoryTuningOverride(ScopedWorkingMemoryTuningOverride&&) = delete;
  ScopedWorkingMemoryTuningOverride& operator=(ScopedWorkingMemoryTuningOverride&&) = delete;

private:
  uint64 m_PreviousSizeBytes = 0;
};

/**
 * @brief Resolves a compiled engine preference against the active developer override.
 */
[[nodiscard]] SIMPLNX_EXPORT uint64 ResolvePreferredWorkingMemoryBytes(uint64 preferredBytes) noexcept;

/**
 * @brief Converts a dataset-specific useful byte count into a bounded preferred fraction.
 *
 * Fractions above one are clamped to the useful size. The minimum never causes
 * the result to exceed usefulBytes. A zero denominator returns zero.
 */
[[nodiscard]] SIMPLNX_EXPORT uint64 ResolveWorkingMemoryFractionBytes(uint64 usefulBytes, uint32 numerator, uint32 denominator, uint64 minimumBytes) noexcept;

/**
 * @brief Reserves the smaller of the resolved preferred size and useful size.
 *
 * CacheMemoryBudgetManager applies the process-wide aggregate one-quarter ceiling
 * and returns the actual granted byte count in the move-only reservation.
 */
[[nodiscard]] SIMPLNX_EXPORT CacheMemoryBudgetManager::WorkingMemoryReservation ReserveWorkingMemory(uint64 preferredBytes, uint64 usefulBytes);

/**
 * @brief Reserves a measured fraction of the dataset-specific useful working set.
 *
 * An active developer tuning override replaces the fraction-derived preference;
 * useful-size clamping and the aggregate one-quarter cache-budget ceiling remain.
 */
[[nodiscard]] SIMPLNX_EXPORT CacheMemoryBudgetManager::WorkingMemoryReservation ReserveWorkingMemoryFraction(uint64 usefulBytes, uint32 numerator, uint32 denominator, uint64 minimumBytes);
} // namespace nx::core::ImageProcessing
