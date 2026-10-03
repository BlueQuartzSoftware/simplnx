#pragma once

#include "IdentifySampleBatchTestUtilities.hpp"
#include "SimplnxCore/Filters/Algorithms/IdentifySampleCCL.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/global_control.h>
#endif
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace IdentifySampleBenchmarkTest
{
using namespace nx::core;
constexpr usize k_Dimension = 200;
constexpr usize k_XyValues = k_Dimension * k_Dimension;
constexpr uint64 k_DefaultBudget = 536870912;
constexpr usize k_RequestedThreads = 8;
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;

/**
 * @brief Parses a positive uint64 field with at most twenty decimal digits.
 * @param text Complete decimal field.
 * @param name Identifies the field in an error message.
 * @return Parsed value.
 * @throws std::invalid_argument If the field is empty, too long, zero, out of range, or contains another character.
 */
inline uint64 ParsePositiveDecimal(std::string_view text, std::string_view name)
{
  if(text.empty() || text.size() > 20 || !std::all_of(text.begin(), text.end(), [](char value) { return value >= '0' && value <= '9'; }))
  {
    throw std::invalid_argument(std::string(name) + " requires one to twenty decimal digits");
  }
  uint64 value = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if(parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || value == 0)
  {
    throw std::invalid_argument(std::string(name) + " requires a positive uint64 value");
  }
  return value;
}

/**
 * @brief Reads one bounded decimal environment field without changing the environment.
 * @param name Environment-variable name.
 * @param fallback Value used only when the variable is absent.
 * @return Parsed value or the fallback.
 * @throws std::invalid_argument If the present value is not a positive uint64 decimal field.
 */
inline uint64 ReadPositiveEnvironment(const char* name, uint64 fallback)
{
  const char* raw = std::getenv(name);
  if(raw == nullptr)
  {
    return fallback;
  }
  usize length = 0;
  while(length <= 20 && raw[length] != '\0')
  {
    length++;
  }
  if(length > 20)
  {
    throw std::invalid_argument(std::string(name) + " exceeds twenty decimal digits");
  }
  return ParsePositiveDecimal(std::string_view(raw, length), name);
}

inline int64 UtcMilliseconds()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline uint64 ProcessId()
{
#ifdef _WIN32
  return static_cast<uint64>(GetCurrentProcessId());
#else
  return static_cast<uint64>(getpid());
#endif
}

// These are process-wide OS requests. Other files, metadata, and device requests can contribute.
struct ProcessIo
{
  bool available = false;
  uint64 error = 0;
  uint64 read = 0;
  uint64 write = 0;
  uint64 other = 0;
};

inline ProcessIo ReadProcessIo() noexcept
{
  ProcessIo result;
#ifdef _WIN32
  IO_COUNTERS counters{};
  if(GetProcessIoCounters(GetCurrentProcess(), &counters) == FALSE)
  {
    result.error = GetLastError();
    return result;
  }
  result.available = true;
  result.read = counters.ReadTransferCount;
  result.write = counters.WriteTransferCount;
  result.other = counters.OtherTransferCount;
#endif
  return result;
}

inline Json ProcessIoDelta(const ProcessIo& before, const ProcessIo& after)
{
  if(!before.available || !after.available)
  {
    return {{"available", false}, {"before_error", before.error}, {"after_error", after.error}, {"scope", "Windows self-process I/O requests; unavailable on other platforms"}};
  }
  REQUIRE(after.read >= before.read);
  REQUIRE(after.write >= before.write);
  REQUIRE(after.other >= before.other);
  return {{"available", true},
          {"read_bytes", after.read - before.read},
          {"write_bytes", after.write - before.write},
          {"other_bytes", after.other - before.other},
          {"scope", "current-process OS requests; includes other files; not mask-only or physical device bytes"}};
}

/**
 * @class BudgetRestore
 * @brief Restores only the current manager budget after test-owned reservations expire.
 */
class BudgetRestore
{
public:
  BudgetRestore()
  : m_Original(CacheMemoryBudgetManager::instance().budgetBytes())
  {
  }
  ~BudgetRestore()
  {
    CacheMemoryBudgetManager::instance().setBudgetBytes(m_Original);
  }
  BudgetRestore(const BudgetRestore&) = delete;
  BudgetRestore& operator=(const BudgetRestore&) = delete;

private:
  uint64 m_Original;
};

inline bool InputValue(usize x, usize y, usize z)
{
  const usize shift = x % 3;
  const bool rectangle = y >= 20 + shift && y <= 179 + shift && z >= 20 && z <= 179;
  return (rectangle && (y != 80 + shift || z != 90)) || (y == 2 && z == 2);
}

inline bool ExpectedValue(usize x, usize y, usize z, bool fillHoles)
{
  const usize shift = x % 3;
  return y >= 20 + shift && y <= 179 + shift && z >= 20 && z <= 179 && (fillHoles || y != 80 + shift || z != 90);
}

template <class T>
DataStructure CreateBenchmarkFixture()
{
  DataStructure data;
  auto* image = ImageGeom::Create(data, IdentifySampleBatchTest::k_ImagePath.getTargetName());
  REQUIRE(image != nullptr);
  image->setDimensions({k_Dimension, k_Dimension, k_Dimension});
  image->setSpacing({1.0F, 1.0F, 1.0F});
  image->setOrigin({0.0F, 0.0F, 0.0F});
  auto* cells = AttributeMatrix::Create(data, "CellData", {k_Dimension, k_Dimension, k_Dimension}, image->getId());
  REQUIRE(cells != nullptr);
  image->setCellData(*cells);
  auto store = DataStoreUtilities::CreateDataStore<T>(data, IdentifySampleBatchTest::k_MaskPath, cells->getShape(), ShapeType{1});
  REQUIRE(store != nullptr);
  auto* mask = DataArray<T>::Create(data, "Mask", store, cells->getId());
  REQUIRE(mask != nullptr);
  auto xy = std::make_unique<T[]>(k_XyValues);
  for(usize z = 0; z < k_Dimension; z++)
  {
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        xy[y * k_Dimension + x] = static_cast<T>(InputValue(x, y, z));
      }
    }
    const auto result = store->copyFromBuffer(z * k_XyValues, nonstd::span<const T>(xy.get(), k_XyValues));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  return data;
}

/**
 * @brief Checks every resident output value against the independent large-fixture predicate.
 * @tparam T Bool or UInt8 mask type.
 * @param data Filter output with the original geometry and mask.
 * @param fillHoles Selects the expected filled-hole value.
 */
template <class T>
void RequireResidentOutput(const DataStructure& data, bool fillHoles)
{
  REQUIRE_NOTHROW(data.getDataRefAs<ImageGeom>(IdentifySampleBatchTest::k_ImagePath));
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath));
  const auto& image = data.getDataRefAs<ImageGeom>(IdentifySampleBatchTest::k_ImagePath);
  const auto& mask = data.getDataRefAs<DataArray<T>>(IdentifySampleBatchTest::k_MaskPath);
  REQUIRE(image.getDimensions() == SizeVec3(k_Dimension, k_Dimension, k_Dimension));
  const ShapeType expectedShape{k_Dimension, k_Dimension, k_Dimension};
  REQUIRE(mask.getTupleShape() == expectedShape);
  REQUIRE(mask.getComponentShape() == ShapeType{1});
  auto xy = std::make_unique<T[]>(k_XyValues);
  uint64 trueCount = 0;
  for(usize z = 0; z < k_Dimension; z++)
  {
    const auto result = mask.getDataStoreRef().copyIntoBuffer(z * k_XyValues, nonstd::span<T>(xy.get(), k_XyValues));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        const T expected = static_cast<T>(ExpectedValue(x, y, z, fillHoles));
        if(xy[y * k_Dimension + x] != expected)
        {
          FAIL("Resident value mismatch at " << x << ',' << y << ',' << z);
        }
        trueCount += static_cast<uint64>(xy[y * k_Dimension + x]);
      }
    }
  }
  REQUIRE(trueCount == (fillHoles ? 5120000ULL : 5119800ULL));
  UnitTest::CheckArraysInheritTupleDims(data);
}
/**
 * @struct LogicalCounters
 * @brief Accumulates CCL slice observations on the filter execution thread.
 * @note The selected mask identity outlives the callback. No callback runs under storage or HDF5 locks.
 * @note extraAllocatedBytes, extraRequestedBytes, and extraGrantedBytes accumulate across batches.
 * @note minimumBatchWidth remains uint64's maximum until the first batch-width event.
 */
struct LogicalCounters
{
  const IDataArray* selected = nullptr;
  uint64 reads = 0;
  uint64 writes = 0;
  uint64 readSuccess = 0;
  uint64 writeSuccess = 0;
  uint64 readValues = 0;
  uint64 writeValues = 0;
  uint64 maximumSpan = 0;
  uint64 active = 0;
  uint64 concurrent = 0;
  uint64 planeLive = 0;
  uint64 xyLive = 0;
  uint64 namedCarrierPeak = 0;
  uint64 allocationEvents = 0;
  uint64 releaseEvents = 0;
  uint64 reservationMin = std::numeric_limits<uint64>::max();
  uint64 reservationMax = 0;
  bool invalid = false;

  uint64 extraLiveBytes = 0;
  uint64 extraPeakBytes = 0;
  uint64 extraAllocatedBytes = 0;
  uint64 extraAllocationCount = 0;
  uint64 extraReleaseCount = 0;
  uint64 extraRequestCount = 0;
  uint64 extraGrantCount = 0;
  uint64 extraRetainCount = 0;
  uint64 extraReservationReleaseCount = 0;
  uint64 extraRequestedBytes = 0;
  uint64 extraRequestedMaxBytes = 0;
  uint64 extraGrantedBytes = 0;
  uint64 extraGrantedMaxBytes = 0;
  uint64 extraRetainedBytes = 0;
  uint64 extraRetainedMaxBytes = 0;
  uint64 batchCount = 0;
  uint64 batchPlanes = 0;
  uint64 minimumBatchWidth = std::numeric_limits<uint64>::max();
  uint64 maximumBatchWidth = 0;
  bool invalidExtraAccounting = false;

  /**
   * @brief Records transfers and actual buffer lifetimes without allocating memory.
   * @param context Non-null pointer to the live counters for the selected mask.
   * @param mask Identifies the source of the event.
   * @param event Identifies the operation or lifetime boundary.
   * @param values Gives elements, bytes, or planes as defined by the event.
   * @param elementBytes Converts values to bytes; admission and width events use one.
   * @param success Reports transfer completion; lifetime events require true.
   */
  static void Observe(void* context, const IDataArray* mask, IdentifySampleSliceEventForTesting event, usize values, usize elementBytes, bool success) noexcept
  {
    auto& counts = *static_cast<LogicalCounters*>(context);
    if(mask != counts.selected)
    {
      return;
    }
    if(elementBytes == 0 || values > std::numeric_limits<uint64>::max() / elementBytes)
    {
      counts.invalid = true;
      return;
    }
    const uint64 bytes = static_cast<uint64>(values) * elementBytes;
    const uint64 reserved = CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes();
    counts.reservationMin = std::min(counts.reservationMin, reserved);
    counts.reservationMax = std::max(counts.reservationMax, reserved);
    switch(event)
    {
    case IdentifySampleSliceEventForTesting::ReadBegin:
    case IdentifySampleSliceEventForTesting::WriteBegin:
      counts.concurrent = std::max(counts.concurrent, ++counts.active);
      counts.maximumSpan = std::max(counts.maximumSpan, static_cast<uint64>(values));
      if(event == IdentifySampleSliceEventForTesting::ReadBegin)
      {
        counts.reads++;
        counts.readValues += values;
      }
      else
      {
        counts.writes++;
        counts.writeValues += values;
      }
      break;
    case IdentifySampleSliceEventForTesting::ReadEnd:
    case IdentifySampleSliceEventForTesting::WriteEnd:
      if(counts.active == 0)
      {
        counts.invalid = true;
        break;
      }
      counts.active--;
      if(event == IdentifySampleSliceEventForTesting::ReadEnd)
      {
        counts.readSuccess += static_cast<uint64>(success);
      }
      else
      {
        counts.writeSuccess += static_cast<uint64>(success);
      }
      break;
    case IdentifySampleSliceEventForTesting::PlaneAllocated:
    case IdentifySampleSliceEventForTesting::XyAllocated: {
      auto& live = event == IdentifySampleSliceEventForTesting::PlaneAllocated ? counts.planeLive : counts.xyLive;
      counts.invalid = counts.invalid || live != 0;
      live = bytes;
      counts.namedCarrierPeak = std::max(counts.namedCarrierPeak, counts.planeLive + counts.xyLive);
      counts.allocationEvents++;
      break;
    }
    case IdentifySampleSliceEventForTesting::PlaneReleased:
    case IdentifySampleSliceEventForTesting::XyReleased: {
      auto& live = event == IdentifySampleSliceEventForTesting::PlaneReleased ? counts.planeLive : counts.xyLive;
      counts.invalid = counts.invalid || live != bytes;
      live = 0;
      counts.releaseEvents++;
      break;
    }
    case IdentifySampleSliceEventForTesting::ExtraBytesRequested:
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::None || counts.extraLiveBytes != 0;
      counts.m_ExtraPhase = ExtraPhase::Requested;
      counts.m_RequestedBytes = bytes;
      counts.m_GrantedBytes = 0;
      counts.m_ExtraOwnerObserved = false;
      counts.m_BatchWidthObserved = false;
      counts.extraRequestCount++;
      counts.addExtraTotal(counts.extraRequestedBytes, bytes);
      counts.extraRequestedMaxBytes = std::max(counts.extraRequestedMaxBytes, bytes);
      break;
    case IdentifySampleSliceEventForTesting::ExtraBytesGranted:
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Requested || bytes > counts.m_RequestedBytes;
      counts.m_ExtraPhase = ExtraPhase::Granted;
      counts.m_GrantedBytes = bytes;
      counts.extraGrantCount++;
      counts.addExtraTotal(counts.extraGrantedBytes, bytes);
      counts.extraGrantedMaxBytes = std::max(counts.extraGrantedMaxBytes, bytes);
      break;
    case IdentifySampleSliceEventForTesting::ExtraBytesRetained:
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Granted || bytes > counts.m_GrantedBytes;
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || (counts.planeLive == 0 ? bytes != 0 : bytes % counts.planeLive != 0);
      counts.m_ExtraPhase = ExtraPhase::Retained;
      counts.extraRetainedBytes = bytes;
      counts.extraRetainCount++;
      counts.extraRetainedMaxBytes = std::max(counts.extraRetainedMaxBytes, bytes);
      break;
    case IdentifySampleSliceEventForTesting::ExtraPlanesAllocated:
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || !success || counts.m_ExtraPhase != ExtraPhase::Retained || counts.m_ExtraOwnerObserved || counts.m_BatchWidthObserved ||
                                      counts.extraLiveBytes != 0 || bytes == 0 || bytes != counts.extraRetainedBytes;
      counts.m_ExtraOwnerObserved = true;
      counts.extraLiveBytes = bytes;
      counts.extraPeakBytes = std::max(counts.extraPeakBytes, bytes);
      counts.addExtraTotal(counts.extraAllocatedBytes, bytes);
      counts.extraAllocationCount++;
      break;
    case IdentifySampleSliceEventForTesting::ExtraPlanesReleased:
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || !success || counts.m_ExtraPhase != ExtraPhase::Retained || !counts.m_ExtraOwnerObserved || bytes == 0 ||
                                      counts.extraLiveBytes != bytes || bytes > counts.extraRetainedBytes;
      counts.extraLiveBytes = 0;
      counts.extraReleaseCount++;
      break;
    case IdentifySampleSliceEventForTesting::BatchWidth: {
      const uint64 expectedWidth = counts.planeLive == 0 ? 1 : 1 + counts.extraRetainedBytes / counts.planeLive;
      counts.invalidExtraAccounting = counts.invalidExtraAccounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Retained || counts.m_BatchWidthObserved || values < 1 ||
                                      values > 8 || values != expectedWidth || counts.extraLiveBytes != counts.extraRetainedBytes;
      counts.m_BatchWidthObserved = true;
      counts.batchCount++;
      counts.addExtraTotal(counts.batchPlanes, values);
      counts.minimumBatchWidth = std::min(counts.minimumBatchWidth, static_cast<uint64>(values));
      counts.maximumBatchWidth = std::max(counts.maximumBatchWidth, static_cast<uint64>(values));
      break;
    }
    case IdentifySampleSliceEventForTesting::ExtraReservationReleased:
      // The token returns its grant only after the extra array releases its values.
      counts.invalidExtraAccounting =
          counts.invalidExtraAccounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Retained || counts.extraLiveBytes != 0 || bytes != counts.extraRetainedBytes;
      counts.extraRetainedBytes = 0;
      counts.m_ExtraPhase = ExtraPhase::None;
      counts.extraReservationReleaseCount++;
      break;
    }
  }

  /**
   * @brief Reports observed counts and peaks with separate extra-plane ownership fields.
   * @return Measurements for the selected mask, excluding total process memory.
   */
  Json describe() const
  {
    return {{"read_calls", reads},
            {"write_calls", writes},
            {"successful_read_calls", readSuccess},
            {"successful_write_calls", writeSuccess},
            {"read_values", readValues},
            {"write_values", writeValues},
            {"maximum_span_values", maximumSpan},
            {"outer_concurrency", concurrent},
            {"original_plane_and_xy_requested_peak_bytes", namedCarrierPeak},
            {"carrier_allocation_events", allocationEvents},
            {"carrier_release_events", releaseEvents},
            {"working_reservation_min_bytes", reservationMin},
            {"working_reservation_max_bytes", reservationMax},
            {"extra_plane_allocation_observation", "actual extra-array ownership; excludes original carriers, CCL records, and backend memory"},
            {"extra_plane_live_bytes", extraLiveBytes},
            {"extra_plane_peak_bytes", extraPeakBytes},
            {"extra_plane_allocated_bytes", extraAllocatedBytes},
            {"extra_plane_allocation_events", extraAllocationCount},
            {"extra_plane_release_events", extraReleaseCount},
            {"extra_request_events", extraRequestCount},
            {"extra_grant_events", extraGrantCount},
            {"extra_retain_events", extraRetainCount},
            {"extra_reservation_release_events", extraReservationReleaseCount},
            {"extra_requested_bytes", extraRequestedBytes},
            {"extra_requested_max_bytes", extraRequestedMaxBytes},
            {"extra_granted_bytes", extraGrantedBytes},
            {"extra_granted_max_bytes", extraGrantedMaxBytes},
            {"extra_retained_live_bytes", extraRetainedBytes},
            {"extra_retained_max_bytes", extraRetainedMaxBytes},
            {"batch_count", batchCount},
            {"batch_planes", batchPlanes},
            {"minimum_batch_width", batchCount == 0 ? 0 : minimumBatchWidth},
            {"maximum_batch_width", maximumBatchWidth},
            {"extra_accounting_invalid", invalidExtraAccounting}};
  }

private:
  /**
   * @enum ExtraPhase
   * @brief Tracks admission order separately from the manager's global reservation total.
   */
  enum class ExtraPhase
  {
    None,      ///< No admission is active.
    Requested, ///< The next event must report the actual grant.
    Granted,   ///< The next event must report the whole-plane reservation.
    Retained   ///< The token remains active until all extra values are released.
  };
  ExtraPhase m_ExtraPhase = ExtraPhase::None;
  uint64 m_RequestedBytes = 0;
  uint64 m_GrantedBytes = 0;
  bool m_ExtraOwnerObserved = false;
  bool m_BatchWidthObserved = false;

  /**
   * @brief Adds an observation without allowing a wrapped total to appear valid.
   * @param total Accumulates the observed quantity.
   * @param amount Gives the increment in the same units as total.
   */
  void addExtraTotal(uint64& total, uint64 amount) noexcept
  {
    if(amount > std::numeric_limits<uint64>::max() - total)
    {
      invalidExtraAccounting = true;
      return;
    }
    total += amount;
  }
};

/**
 * @brief Checks actual batch widths and extra owners for the 200-cube fixture.
 * @tparam T Bool or UInt8 mask element type.
 * @param counts Contains observations from one successful CCL execution.
 */
template <class T>
void RequireFullWidthYzBatchObservations(const LogicalCounters& counts)
{
  constexpr uint64 expectedBatches = k_Dimension / 8;
  constexpr uint64 expectedExtraBytes = 7 * k_XyValues * sizeof(T);
  static_assert(k_Dimension % 8 == 0);
  REQUIRE_FALSE(counts.invalidExtraAccounting);
  REQUIRE(counts.extraLiveBytes == 0);
  REQUIRE(counts.extraRetainedBytes == 0);
  REQUIRE(counts.extraAllocationCount == expectedBatches);
  REQUIRE(counts.extraReleaseCount == expectedBatches);
  REQUIRE(counts.extraRequestCount == expectedBatches);
  REQUIRE(counts.extraGrantCount == expectedBatches);
  REQUIRE(counts.extraRetainCount == expectedBatches);
  REQUIRE(counts.extraReservationReleaseCount == expectedBatches);
  REQUIRE(counts.extraAllocatedBytes == expectedBatches * expectedExtraBytes);
  REQUIRE(counts.extraPeakBytes == expectedExtraBytes);
  REQUIRE(counts.extraRequestedBytes == expectedBatches * expectedExtraBytes);
  REQUIRE(counts.extraRequestedMaxBytes == expectedExtraBytes);
  REQUIRE(counts.extraGrantedBytes == expectedBatches * expectedExtraBytes);
  REQUIRE(counts.extraGrantedMaxBytes == expectedExtraBytes);
  REQUIRE(counts.extraRetainedMaxBytes == expectedExtraBytes);
  REQUIRE(counts.extraPeakBytes <= counts.extraRetainedMaxBytes);
  REQUIRE(counts.extraRetainedMaxBytes <= counts.extraGrantedMaxBytes);
  REQUIRE(counts.batchCount == expectedBatches);
  REQUIRE(counts.batchPlanes == k_Dimension);
  REQUIRE(counts.minimumBatchWidth == 8);
  REQUIRE(counts.maximumBatchWidth == 8);
}

} // namespace IdentifySampleBenchmarkTest
