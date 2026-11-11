#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <utility>

/**
 * @namespace nx::core::orientation::detail
 * @brief Provides private admitted buffers and serial tuple gathers for face-orientation algorithms.
 */
namespace nx::core::orientation::detail
{
/**
 * @struct LookupPlan
 * @brief Keeps the shared reservation alive until the caller releases its lookup and face buffers.
 */
struct LookupPlan
{
  CacheMemoryBudgetManager::WorkingMemoryReservation Reservation;
  usize FaceCapacity = 0;
  usize PageCapacity = 0;
  bool CacheWholeTables = false;
};

/**
 * @struct LookupSizes
 * @brief Names the tuple counts and payload widths used to plan both face-orientation algorithms.
 */
struct LookupSizes
{
  usize FaceCount = 0;
  usize FeatureCount = 0;
  usize EnsembleCount = 0;
  usize OrientationComponents = 0;
  usize FaceBytes = 0;
};

/**
 * @brief Plans simultaneous face, lookup, order, and page payloads from the actual shared grant.
 * @param sizes Supplies tuple counts and per-tuple payload widths.
 * @param errorCode Identifies the caller's allocation-planning error.
 * @return An admitted plan, or an error when one face and the minimum lookup pages do not fit.
 */
inline Result<LookupPlan> PlanLookups(const LookupSizes& sizes, int32 errorCode)
{
  constexpr usize maximumFaces = 262144;
  constexpr usize maximumPageTuples = 4096;
  constexpr uint64 maximumBytes = (std::numeric_limits<usize>::max)();
  const usize desiredFaces = (std::min)(sizes.FaceCount, maximumFaces);
  const usize orientationBytes = sizes.OrientationComponents * sizeof(float32);
  const usize featureBytes = sizeof(int32) + orientationBytes;
  const usize pageBytes = featureBytes + sizeof(uint32);
  const usize gatheredFaceBytes = sizes.FaceBytes + (2 * (pageBytes + sizeof(usize)));
  auto& budget = CacheMemoryBudgetManager::instance();
  LookupPlan plan;

  // A complete cache is optional. Its reservation also covers the ordinary face block.
  if(sizes.FeatureCount <= maximumBytes / featureBytes && sizes.EnsembleCount <= maximumBytes / sizeof(uint32))
  {
    const uint64 featurePayload = static_cast<uint64>(sizes.FeatureCount) * featureBytes;
    const uint64 ensemblePayload = static_cast<uint64>(sizes.EnsembleCount) * sizeof(uint32);
    const uint64 facePayload = static_cast<uint64>(desiredFaces) * sizes.FaceBytes;
    if(featurePayload <= maximumBytes - ensemblePayload && featurePayload + ensemblePayload <= maximumBytes - facePayload)
    {
      plan.Reservation = budget.tryReserveWorkingMemoryWithoutEviction(featurePayload + ensemblePayload + facePayload);
      if(plan.Reservation.sizeBytes() != 0)
      {
        plan.FaceCapacity = desiredFaces;
        plan.CacheWholeTables = true;
        Result<LookupPlan> result;
        result.value() = std::move(plan);
        return result;
      }
    }
  }

  const usize desiredPages = (std::max)(usize{1}, (std::min)({maximumPageTuples, desiredFaces * 2, (std::max)(sizes.FeatureCount, sizes.EnsembleCount)}));
  const uint64 desiredBytes = (static_cast<uint64>(desiredFaces) * gatheredFaceBytes) + (static_cast<uint64>(desiredPages) * pageBytes);
  plan.Reservation = budget.reserveWorkingMemory(desiredBytes);
  const uint64 grantedBytes = plan.Reservation.sizeBytes();
  const usize minimumBytes = gatheredFaceBytes + pageBytes;
  if(grantedBytes < minimumBytes)
  {
    return MakeErrorResult<LookupPlan>(errorCode, fmt::format("Face-orientation buffers require at least {} working bytes, but the shared cache budget admitted {}. "
                                                              "Increase the cache memory budget or release other working buffers.",
                                                              minimumBytes, grantedBytes));
  }

  // Keep the ordinary face block when it fits. Smaller grants retain useful pages as well as faces.
  if(grantedBytes >= static_cast<uint64>(desiredFaces) * gatheredFaceBytes + pageBytes)
  {
    plan.FaceCapacity = desiredFaces;
  }
  else
  {
    plan.FaceCapacity = (std::max)(usize{1}, static_cast<usize>(grantedBytes / (gatheredFaceBytes + (2 * pageBytes))));
  }
  plan.PageCapacity = (std::min)(desiredPages, static_cast<usize>((grantedBytes - (static_cast<uint64>(plan.FaceCapacity) * gatheredFaceBytes)) / pageBytes));
  plan.Reservation.shrinkTo((static_cast<uint64>(plan.FaceCapacity) * gatheredFaceBytes) + (static_cast<uint64>(plan.PageCapacity) * pageBytes));
  Result<LookupPlan> result;
  result.value() = std::move(plan);
  return result;
}

/**
 * @brief Reads a lookup table while preserving store diagnostics and adding the selected path on failure.
 * @tparam T Specifies the lookup value type.
 * @param store Supplies the selected table.
 * @param values Receives the admitted complete table.
 * @param errorCode Identifies the caller's lookup-read error.
 * @param path Identifies the selected array for diagnostics.
 * @return The store result, with path context on failure.
 */
template <typename T>
Result<> ReadLookupTable(const AbstractDataStore<T>& store, nonstd::span<T> values, int32 errorCode, const DataPath& path)
{
  auto result = store.copyIntoBuffer(0, values);
  if(result.invalid())
  {
    return MergeResults(std::move(result), MakeErrorResult(errorCode, fmt::format("Could not read orientation lookup array '{}'. Check that its data store is available.", path.toString())));
  }
  return result;
}

/**
 * @brief Gathers positive tuple IDs in page order into stable caller-owned output slots.
 * @tparam T Specifies the lookup value type.
 * @tparam Components Specifies the complete tuple width.
 * @param store Supplies the selected table.
 * @param indexes Supplies one tuple ID per destination slot. Nonpositive IDs produce zero values.
 * @param order Supplies reusable sort positions, one per request.
 * @param gathered Receives complete tuples in the original request order.
 * @param page Supplies an admitted complete-tuple read page.
 * @param shouldCancel Requests cancellation between page reads.
 * @param errorCode Identifies the caller's lookup-read error.
 * @param path Identifies the selected array for diagnostics.
 * @param reuseOrder Reuses positions already sorted for the identical request IDs.
 * @return Store warnings or an error with the selected path. Cancellation leaves unused output slots incomplete.
 */
template <typename T, usize Components>
Result<> GatherLookupTuples(const AbstractDataStore<T>& store, nonstd::span<const int32> indexes, nonstd::span<usize> order, nonstd::span<T> gathered, nonstd::span<T> page,
                            const std::atomic_bool& shouldCancel, int32 errorCode, const DataPath& path, bool reuseOrder = false)
{
  constexpr usize maximum = (std::numeric_limits<usize>::max)();
  if(indexes.size() > maximum / Components || gathered.size() < indexes.size() * Components || order.size() < indexes.size() || page.empty() || page.size() % Components != 0 ||
     store.getNumberOfComponents() != Components || store.getNumberOfTuples() > maximum / Components)
  {
    return MakeErrorResult(
        errorCode, fmt::format("Orientation lookup array '{}' requires {} components and complete tuple buffers. Check the selected array and working buffer sizes.", path.toString(), Components));
  }
  std::fill_n(gathered.begin(), indexes.size() * Components, T{});
  usize requests = 0;
  for(usize slot = 0; slot < indexes.size(); ++slot)
  {
    if(indexes[slot] > 0)
    {
      if(static_cast<usize>(indexes[slot]) >= store.getNumberOfTuples())
      {
        return MakeErrorResult(errorCode, fmt::format("Orientation lookup array '{}' contains {} tuples, but requested tuple {} is outside [0, {}). Check the selected feature and phase arrays.",
                                                      path.toString(), store.getNumberOfTuples(), indexes[slot], store.getNumberOfTuples()));
      }
      if(!reuseOrder)
      {
        order[requests] = slot;
      }
      ++requests;
    }
  }
  if(!reuseOrder)
  {
    std::ranges::sort(order.first(requests), [&indexes](usize left, usize right) { return indexes[left] < indexes[right]; });
  }
  const usize pageTuples = page.size() / Components;
  usize loadedPage = maximum;
  Result<> result;
  for(usize request = 0; request < requests; ++request)
  {
    const usize slot = order[request];
    const usize tuple = static_cast<usize>(indexes[slot]);
    const usize first = (tuple / pageTuples) * pageTuples;
    if(first != loadedPage)
    {
      if(shouldCancel)
      {
        return result;
      }
      const usize count = (std::min)(pageTuples, store.getNumberOfTuples() - first);
      result = MergeResults(std::move(result), store.copyIntoBuffer(first * Components, page.first(count * Components)));
      if(result.invalid())
      {
        return MergeResults(std::move(result), MakeErrorResult(errorCode, fmt::format("Could not read orientation lookup array '{}' at tuple {} ({} tuples). Check that its data store is available.",
                                                                                      path.toString(), first, count)));
      }
      loadedPage = first;
    }
    std::ranges::copy(page.subspan((tuple - first) * Components, Components), gathered.subspan(slot * Components, Components).begin());
  }
  return result;
}
} // namespace nx::core::orientation::detail
